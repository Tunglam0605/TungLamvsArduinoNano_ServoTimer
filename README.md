# TungLamvsArduinoNano_ServoTimer

Firmware register-level cho Arduino Nano ATmega328P điều khiển 9 bia servo với 3 phần thi.

## Nguyên tắc triển khai

- Không dùng `delay()`, `millis()`, `Servo.h`, `digitalWrite()`, `digitalRead()` hay `pinMode()`.
- Timer1 16-bit, prescaler /8: phát xung tuần tự cho 9 servo trong frame 20 ms.
- Timer2 CTC, prescaler /64, OCR2A=249: system tick 1 ms.
- Timer0 free-running: entropy cho lựa chọn ngẫu nhiên ở phần 2.
- FSM chạy ở main loop; ISR chỉ làm timing tối thiểu.
- Góc bia hạ: 0°; góc bia dựng: 90°.
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
| MODE potentiometer wiper | A2/ADC2 |
| Part 2 - Left button | A3 |
| Part 2 - Right button | A4 |
| START button | A5 |

Biến trở MODE: nối hai đầu ngoài vào 5 V và GND, chân giữa (wiper) vào A2.
Các nút A3..A5 dùng active-low và internal pull-up.

## Phần thi 1

6 lượt, mỗi lượt hai bia dựng 8 s, sau đó tất cả hạ 10 s:

1. 1 + 3
2. 2 + 4
3. 2 + 3
4. 1 + 4
5. 1 + 2
6. 3 + 4

## Phần thi 2

- Khi START: bia trái và phải dựng, LED tương ứng sáng, bia giữa hạ.
- Nhấn một trong hai nút trái/phải sẽ kích hoạt một lần lựa chọn ngẫu nhiên.
- Ngẫu nhiên chọn bia trái hoặc phải để hạ; LED của bia được chọn tắt.
- Bia giữa dựng trong 25 s rồi hạ.
- Kết thúc lượt: tất cả bia hạ.

## Phần thi 3

- Bia 1: lặp 3 lần, mỗi lần dựng 8 s rồi hạ 7 s.
- Bia 2: dựng một lần duy nhất trong 15 s rồi hạ.
- **Giả định hiện tại:** bia 2 bắt đầu dựng đồng thời với lần đầu tiên bia 1.

## Chọn phần thi

- Biến trở MODE chỉ thay đổi phần thi khi hệ thống IDLE.
- ADC 0..340 chọn Part 1, 341..682 chọn Part 2, 683..1023 chọn Part 3.
- Firmware lấy mẫu mỗi 10 ms, yêu cầu vùng mới ổn định 50 ms và có hysteresis để chống nhảy mode gần biên.
- Ba LED D11/D12/D13 báo phần thi đang chọn.
- START ở A5 bắt đầu phần thi hiện tại.
- Khi đang RUNNING, thay đổi biến trở và START không ảnh hưởng bài đang chạy; mode mới được áp dụng khi trở lại IDLE.

## Serial debug

USART0 phát log ở 115200 baud mỗi 200 ms, gồm giá trị ADC, mode, trạng thái và sự kiện nút:

```text
ADC=512 MODE=2 STATE=IDLE BTN=-
ADC=512 MODE=2 STATE=RUNNING BTN=START
```

Gửi `START` (không phân biệt chữ hoa/thường) từ Serial Monitor để bắt đầu mode
đang chọn, tương đương nhấn nút START ở A5. UART RX dùng ngắt và bộ đệm nên
không chặn main loop.

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
