## 📁 CSV Merger Tool

간단한 Windows 전용 CSV 병합 툴입니다.

### ✅ 기능

* 기획자 폴더의 CSV 파일을 기준으로
  개발자 폴더의 동일한 이름의 CSV 파일을 수정합니다.
* **기획자 파일의 수치만 덮어쓰기**하고,
  **개발자 파일에 추가된 컬럼은 유지**합니다.
* 각 행은 **순서대로 대응**되며, `Level`, `Damage` 등의 수치만 변경됩니다.

### 🛠 사용 방법

1. `CSV_Merger.exe` 실행
2. **기획자 CSV 폴더** 선택
3. **개발자 CSV 폴더** 선택
4. 결과는 **개발자 파일이 직접 수정**됩니다

### ⚙️ 빌드 환경

* C++17 이상
* Windows API 사용 (`<filesystem>`, `<shlobj.h>`)
* Visual Studio Release 빌드 (x64, 유니코드 설정)
