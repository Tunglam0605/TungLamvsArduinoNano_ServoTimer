# TungLamvsArduinoNano_ServoTimer

Firmware register-level cho Arduino Nano ATmega328P điều khiển 9 bia servo với 3 phần thi.

## Nguyên tắc triển khai

- Không dùng `delay()`, `millis()`, `Servo.h`, `digitalWrite()`, `digitalRead()` hay `pinMode()`.
- Timer1 16-bit, prescaler /8: phát xung tuần tự cho 9 servo trong frame 20 ms.
- Timer2 CTC, prescaler /64, OCR2A=249: system tick 1 ms.
- Timer0 free-running: entropy cho lựa chọn ngẫu nhiên ở phần 2.
- FSM chạy ở main loop; ISR chỉ làm timing tối thiểu.
- Góc bia hạ: 0°; góc bia dựng: 90°.
- Biến trở A6 chỉnh ramp chung cho 9 servo; hành trình 0° -> 90° khoảng 0,2..2,0 s.
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
| Mode LED 1 | D11 |
| Mode LED 2 | D12 |
| Mode LED 3 | D13 |
| Part 2 - Left LED | A0 |
| Part 2 - Right LED | A1 |
| Part 1 maintained switch | A2 |
| Part 2 - Left button | A3 |
| Part 2 - Right button | A4 |
| Part 2 maintained switch | A5 |
| Servo speed potentiometer wiper | A6/ADC6 |
| Part 3 maintained switch | A7/ADC7 |

Biến trở tốc độ: nối hai đầu ngoài vào 5 V và GND, chân giữa (wiper) vào A6.
Nút tự giữ Phần 1 và Phần 2 nối lần lượt A2/A5 xuống GND, dùng pull-up nội.
Nút tự giữ Phần 3 nối A7 xuống GND và cần điện trở kéo lên 10 kΩ từ A7 lên 5 V.
Hai nút A3/A4 của Phần 2 vẫn là active-low với pull-up nội.

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
- Nhấn một trong hai nút trái/phải sẽ kích hoạt một lần lựa chọn ngẫu nhiên.
- Ngẫu nhiên chọn bia trái hoặc phải để hạ; LED của bia được chọn tắt.
- Bia giữa dựng trong 25 s rồi hạ.
- Kết thúc lượt: tất cả bia hạ.

## Phần thi 3

- Bia 1: lặp 3 lần, mỗi lần dựng 8 s rồi hạ 7 s.
- Bia 2: dựng một lần duy nhất trong 15 s rồi hạ.
- **Giả định hiện tại:** bia 2 bắt đầu dựng đồng thời với lần đầu tiên bia 1.

## Chọn phần thi

- Có ba nút tự giữ tương ứng Phần 1, Phần 2 và Phần 3.
- Chỉ khi đúng một nút ON, phần thi tương ứng mới tự chạy.
- Nếu không có nút nào ON hoặc có hai/ba nút cùng ON, tất cả bia hạ và không phần thi nào chạy.
- Nếu trạng thái nút trở thành không hợp lệ khi đang RUNNING, bài hiện tại dừng và tất cả bia hạ.
- Mỗi lần chuyển OFF -> ON chỉ chạy một lần. Muốn chạy lại cùng phần phải đưa nút về OFF rồi ON lại.
- LED D11/D12/D13 phản ánh trực tiếp trạng thái ON của ba nút tự giữ.

## Serial debug

USART0 phát log ở 115200 baud mỗi 200 ms, gồm tốc độ, ba trạng thái chọn, trạng thái chạy và bia đang dựng:

```text
SPEED_ADC=600 MOVE90_MS=320 SW=000 SELECT=NONE STATE=IDLE BTN=- UP=-
SPEED_ADC=600 MOVE90_MS=320 SW=010 SELECT=2 STATE=RUNNING BTN=- UP=P2L,P2R
SPEED_ADC=600 MOVE90_MS=320 SW=110 SELECT=INVALID STATE=IDLE BTN=- UP=-
```

`SW` lần lượt là trạng thái Phần 1/2/3; `1` là ON. `SELECT=INVALID` nghĩa là có nhiều nút cùng ON.
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
  servo_engine.h   Timer1 servo API
  input.h          Button events
  target.h         Logical target API
  competition.h    Competition FSM API
src/
  main.cpp
  system_tick.cpp
  servo_engine.cpp
  input.cpp
  target.cpp
  competition.cpp
```

## Lưu ý nguồn

Không cấp dòng cho 9 servo qua regulator của Arduino Nano. Dùng nguồn 5 V riêng cho servo, cấp trực tiếp vào rail servo và bắt buộc nối chung GND với Nano.
