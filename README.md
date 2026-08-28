# TungLamvsArduinoNano_ServoTimer

Firmware register-level cho Arduino Nano ATmega328P điều khiển 9 bia servo với 3 phần thi.

## Nguyên tắc triển khai

- Không dùng `delay()`, `millis()`, `Servo.h`, `digitalWrite()`, `digitalRead()` hay `pinMode()`.
- Timer1 16-bit, prescaler /8: phát xung tuần tự cho 9 servo trong frame 20 ms.
- Timer2 CTC, prescaler /64, OCR2A=249: system tick 1 ms.
- Timer0 free-running: entropy cho lựa chọn ngẫu nhiên ở phần 2.
- FSM chạy ở main loop; ISR chỉ làm timing tối thiểu.
- LED tích hợp D13 nháy heartbeat 5 Hz để báo vòng lặp chính vẫn hoạt động.
- Góc bia hạ và góc khởi động: 30°; góc bia dựng: 120°.
- Biến trở A6 chỉnh ramp chung cho 9 servo; hành trình 30° -> 120° khoảng 0,2..2,0 s.
- Nguồn servo phải dùng 5 V ngoài đủ dòng và nối chung GND với Nano.

## Pin mapping

| Chức năng | Nano pin |
|---|---|
| Part 1 - Target 1 | D2 |
| Part 1 - Target 2 | D3 |
| Part 1 - Target 3 | D4 |
| Part 1 - Target 4 | D5 |
| Part 2 - Left | D6 |
| Part 2 - Center | D7 |
| Part 2 - Right | D8 |
| Part 3 - Target 1 | D9 |
| Part 3 - Target 2 | D10 |
| Part 1 maintained switch | A0 |
| Part 2 maintained switch | A1 |
| Part 3 maintained switch | A2 |
| Mode LED 1 | A3 |
| Mode LED 2 | A4 |
| Mode LED 3 | A5 |
| Servo speed potentiometer wiper | A6/ADC6 |
| Part 2 - Left LED | D11 |
| Part 2 - Right LED | D12 |
| System heartbeat / Nano built-in LED | D13 |

Biến trở tốc độ: nối hai đầu ngoài vào 5 V và GND, chân giữa (wiper) vào A6.
Ba nút tự giữ Phần 1/2/3 nối lần lượt A0/A1/A2 xuống GND và đều dùng pull-up nội.
LED báo Mode 1/2/3 nối lần lượt A3/A4/A5 qua điện trở 220–330 ohm xuống GND.
LED trái/phải của Phần 2 nối D11/D12 qua điện trở 220–330 ohm xuống GND.
D13 dùng LED tích hợp làm heartbeat: nháy liên tục 5 Hz để báo firmware đang hoạt động; A7 hiện không sử dụng.

## Phần thi 1

6 lượt, mỗi lượt hai bia dựng 8 s, sau đó tất cả hạ 10 s:

1. 1 + 3
2. 2 + 4
3. 2 + 3
4. 1 + 4
5. 1 + 2
6. 3 + 4

## Phần thi 2

- Khi chỉ nút tự giữ Phần 2 ON: bia trái và phải dựng, LED tương ứng sáng, bia giữa hạ.
- Sau khi Mode 2 đã khóa, nút Phần 1 trở thành nút trái và nút Phần 3 trở thành nút phải.
- Bật một trong hai nút trái/phải sẽ kích hoạt một lần lựa chọn ngẫu nhiên.
- Ngẫu nhiên chọn bia trái hoặc phải để hạ; LED của bia được chọn tắt.
- Bia giữa dựng trong 25 s rồi hạ.
- Kết thúc lượt: tất cả bia hạ.

## Phần thi 3

- Bia 1: lặp 3 lần, mỗi lần dựng 8 s rồi hạ 7 s.
- Bia 2: dựng một lần duy nhất trong 15 s rồi hạ.
- **Giả định hiện tại:** bia 2 bắt đầu dựng đồng thời với lần đầu tiên bia 1.

## Chọn phần thi

- Hệ thống chỉ sẵn sàng chọn mode sau khi đã đọc được cả ba nút OFF (`SW=000`).
- Sau trạng thái `000`, nút đầu tiên ON một mình sẽ khóa Phần 1, Phần 2 hoặc Phần 3 tương ứng.
- Khi một mode đã khóa, hai nút còn lại không thể đổi mode. Riêng trong Mode 2, chúng được dùng làm nút trái/phải.
- Tắt nút đã khóa sẽ kết thúc sớm bài hiện tại và hạ toàn bộ bia.
- Khi bài tự kết thúc hoặc bị tắt, hệ thống chờ cả ba nút trở về OFF trước khi cho phép chọn mode tiếp theo.
- Nếu hai/ba nút cùng bật trước khi một mode được khóa, hệ thống không chọn mode và vẫn yêu cầu trở lại `000`.
- LED A3/A4/A5 sáng liên tục khi Mode 1/2/3 tương ứng đang RUNNING.
- Khi bài kết thúc hoặc bị hủy mà còn nút ON, LED của mode vừa chạy nháy mỗi 500 ms để yêu cầu đưa cả ba nút về OFF.
- Khi cả ba nút đã OFF, LED tắt và hệ thống mới cho phép chọn mode tiếp theo.
- Hai nút phụ trong Mode 2 không làm thay đổi LED mode.

## Serial debug

USART0 phát log ở 115200 baud mỗi 200 ms, gồm tốc độ, ba trạng thái chọn, trạng thái chạy và bia đang dựng:

```text
SPEED_ADC=600 MOVE90_MS=320 SW=000 LOCK=- STATE=IDLE UP=-
SPEED_ADC=600 MOVE90_MS=320 SW=010 LOCK=2 STATE=RUNNING UP=P2L,P2R
SPEED_ADC=600 MOVE90_MS=320 SW=110 LOCK=2 STATE=RUNNING UP=P2R,P2C
SPEED_ADC=600 MOVE90_MS=320 SW=100 LOCK=2 STATE=WAIT_ALL_OFF UP=-
```

`SW` lần lượt là trạng thái vật lý của nút 1/2/3; `1` là ON. `LOCK` là mode đã khóa.
`UP` liệt kê các bia đang dựng; các bia không xuất hiện trong danh sách đang hạ.
Tên bia lần lượt là P1T1..P1T4, P2L/P2C/P2R và P3T1/P3T2.

## Build

Dự án dùng PlatformIO AVR thuần, không dùng Arduino framework:

```bash
pio run
```

Nạp firmware:

```bash
pio run -t upload
```

## Cấu trúc

```text
include/
  board.h          Pin/register mapping
  config.h         Timing constants
  system_tick.h    Timer2 tick API
  heartbeat.h      D13 system-alive heartbeat
  servo_engine.h   Timer1 servo API
  input.h          Maintained competition switch inputs
  target.h         Logical target API
  competition.h    Competition FSM API
  debug_serial.h   USART0 debug API
src/
  main.cpp
  system_tick.cpp
  heartbeat.cpp
  servo_engine.cpp
  input.cpp
  target.cpp
  competition.cpp
  debug_serial.cpp
```

## Lưu ý nguồn

Không cấp dòng cho 9 servo qua regulator của Arduino Nano. Dùng nguồn 5 V riêng cho servo, cấp trực tiếp vào rail servo và bắt buộc nối chung GND với Nano.
