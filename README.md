# BTL_MHH_HK251
Bài tập lớn Mô hình hóa - Petri Nets - HK251

## Yêu cầu hệ thống 
- Hệ điều hành: Linux (Ubuntu) hoặc Windows chạy WSL (Windows Subsystem for Linux).
- Thư viện bắt buộc: **CUDD (Colorado University Decision Diagram)**.
  Chương trình được cấu hình để tìm thư viện CUDD tại thư mục `$HOME/local` (Local Installation).

## Cách biên dịch và chạy program
### Biên dịch
** g++ -o app main.cpp petri.cpp optimization.cpp tinyxml2.cpp -lcudd -lglpk -DUSE_GLPK**

### Chạy program
- Bước 1: Kiểm tra coi Linux có tìm thấy thư viện CUDD không
- Bước 2: Chạy lệnh **./app** (file test.pnml) hoặc **./app tên_file.pnml**
