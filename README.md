# BTL_MHH_HK251
Bài tập lớn Mô hình hóa - Petri Nets - HK251

## Yêu cầu hệ thống 
- Hệ điều hành: Linux (Ubuntu) hoặc Windows chạy WSL (Windows Subsystem for Linux).
- Trình biên dịch:`g++` (hỗ trợ chuẩn C++11 trở lên).
- Các thư viện bên ngoài:**
  **CUDD (Colorado University Decision Diagram)**: Dùng cho các thuật toán Symbolic (Task 3, 4, 5).
  **GLPK (GNU Linear Programming Kit)**: Dùng để kiểm chứng Deadlock bằng quy hoạch tuyến tính (Task 4).
  **TinyXML2**: Đã được tích hợp sẵn trong mã nguồn (`tinyxml2.cpp`) và (`tinyxml2.h`), không cần cài đặt, tải qua link Github: **https://github.com/leethomason/tinyxml2**
## Cách biên dịch và chạy program (nên cài các thư viện trên trước khi biên dịch)
Có thể cài các thư viện bằng lệnh sau:
**sudo apt-get update**
**sudo apt-get install build-essential libcudd-dev libglpk-dev**

### Biên dịch bằng lệnh sau:
**g++ -o app main.cpp petri.cpp optimization.cpp tinyxml2.cpp -lcudd -lglpk -DUSE_GLPK**

### Chạy program
- Bước 1: Kiểm tra xem WSL đã chứa các thư viện trên hay chưa bằng lệnh: **dpkg -l | grep -E "libcudd|libglpk"**
- Bước 2: Chạy lệnh **./app** (mặc định sẽ chạy file test.pnml) hoặc **./app tên_file.pnml** (để chạy các file pnml khác)
