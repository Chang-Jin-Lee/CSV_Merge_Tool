#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "Shell32.lib")
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

std::wstring SelectFolder(const std::wstring& title) {
    BROWSEINFOW bi = { 0 };
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != nullptr) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            return path;
        }
    }
    return L"";
}

std::vector<std::string> SplitCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) {
        result.push_back(cell);
    }
    return result;
}

std::string JoinCSVLine(const std::vector<std::string>& cells) {
    std::string line;
    for (size_t i = 0; i < cells.size(); ++i) {
        line += cells[i];
        if (i < cells.size() - 1) line += ',';
    }
    return line;
}

void OverwriteRowsByOrder(const fs::path& plannerPath, const fs::path& devPath) {
    std::ifstream plannerFile(plannerPath);
    std::ifstream devFile(devPath);
    if (!plannerFile || !devFile) {
        std::cerr << "[열기 실패] 파일: " << plannerPath << " 또는 " << devPath << std::endl;
        return;
    }

    std::string plannerHeaderLine, devHeaderLine;
    std::getline(plannerFile, plannerHeaderLine);
    std::getline(devFile, devHeaderLine);

    auto plannerHeaders = SplitCSVLine(plannerHeaderLine);
    auto devHeaders = SplitCSVLine(devHeaderLine);

    std::vector<std::vector<std::string>> plannerRows;
    std::vector<std::vector<std::string>> devRows;

    std::string line;
    while (std::getline(plannerFile, line)) {
        plannerRows.push_back(SplitCSVLine(line));
    }
    while (std::getline(devFile, line)) {
        devRows.push_back(SplitCSVLine(line));
    }

    // 헤더 매핑: plannerHeader → devHeader 의 인덱스를 찾음
    std::vector<int> plannerToDevIndex;
    for (const auto& colName : plannerHeaders) {
        auto it = std::find(devHeaders.begin(), devHeaders.end(), colName);
        if (it != devHeaders.end()) {
            plannerToDevIndex.push_back(std::distance(devHeaders.begin(), it));
        }
        else {
            plannerToDevIndex.push_back(-1); // 개발자에 없는 컬럼은 무시
        }
    }

    // devRows의 기존 행 업데이트
    size_t rowsToUpdate = min(plannerRows.size(), devRows.size());
    for (size_t i = 0; i < rowsToUpdate; ++i) {
        for (size_t j = 0; j < plannerHeaders.size(); ++j) {
            int targetIdx = plannerToDevIndex[j];
            if (targetIdx != -1 && targetIdx < devRows[i].size() && j < plannerRows[i].size()) {
                devRows[i][targetIdx] = plannerRows[i][j];
            }
        }
    }

    // 기획자 행이 더 많으면 추가
    for (size_t i = devRows.size(); i < plannerRows.size(); ++i) {
        std::vector<std::string> newRow(devHeaders.size(), "");
        for (size_t j = 0; j < plannerHeaders.size(); ++j) {
            int targetIdx = plannerToDevIndex[j];
            if (targetIdx != -1 && j < plannerRows[i].size()) {
                newRow[targetIdx] = plannerRows[i][j];
            }
        }
        devRows.push_back(newRow);
    }

    // 저장
    std::ofstream outFile(devPath);
    if (!outFile) {
        std::cerr << "[쓰기 실패] " << devPath << std::endl;
        return;
    }

    outFile << JoinCSVLine(devHeaders) << "\n";
    for (const auto& row : devRows) {
        outFile << JoinCSVLine(row) << "\n";
    }
}


int main() {
    std::wstring plannerDir = SelectFolder(L"작업한 디렉토리를 입력해주세요 (기획자)");
    std::wstring devDir = SelectFolder(L"합칠 디렉토리를 입력해주세요 (개발자)");

    if (plannerDir.empty() || devDir.empty()) {
        MessageBoxA(nullptr, "모든 폴더를 선택해야 합니다.", "오류", MB_OK | MB_ICONERROR);
        return 1;
    }

    int count = 0;
    for (const auto& file : fs::directory_iterator(plannerDir)) {
        if (!file.is_regular_file() || file.path().extension() != ".csv")
            continue;

        fs::path plannerPath = file.path();
        fs::path devPath = fs::path(devDir) / plannerPath.filename();

        // 정확히 같은 이름의 파일만 처리
        if (!fs::exists(devPath)) {
            std::wcerr << L"[스킵] 개발자 폴더에 해당 파일 없음: " << plannerPath.filename().c_str() << std::endl;
            continue;
        }

        try {
            OverwriteRowsByOrder(plannerPath, devPath);
            count++;
        }
        catch (...) {
            std::wcerr << L"[오류] 처리 중 예외 발생: " << plannerPath.filename().c_str() << std::endl;
        }
    }

    std::string message = std::to_string(count) + "개 파일 병합 완료!";
    MessageBoxA(nullptr, message.c_str(), "완료", MB_OK | MB_ICONINFORMATION);
    return 0;
}
