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
    if (!plannerFile || !devFile) return;

    std::string plannerHeaderLine, devHeaderLine;
    std::getline(plannerFile, plannerHeaderLine);
    std::getline(devFile, devHeaderLine);

    auto plannerHeaders = SplitCSVLine(plannerHeaderLine);
    auto devHeaders = SplitCSVLine(devHeaderLine);
    size_t overwriteColumnCount = plannerHeaders.size();

    std::vector<std::vector<std::string>> plannerRows;
    std::vector<std::vector<std::string>> devRows;

    std::string line;
    while (std::getline(plannerFile, line)) {
        plannerRows.push_back(SplitCSVLine(line));
    }
    while (std::getline(devFile, line)) {
        devRows.push_back(SplitCSVLine(line));
    }

    if (plannerRows.size() != devRows.size()) {
        std::cerr << "줄 수가 다릅니다. planner: " << plannerRows.size() << ", dev: " << devRows.size() << std::endl;
        return;
    }

    // 덮어쓰기
    for (size_t i = 0; i < plannerRows.size(); ++i) {
        for (size_t j = 0; j < overwriteColumnCount && j < devRows[i].size(); ++j) {
            devRows[i][j] = plannerRows[i][j];
        }
    }

    // 결과 저장
    std::ofstream outFile(devPath);
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
        if (file.path().extension() != ".csv") continue;

        fs::path plannerPath = file.path();
        fs::path devPath = fs::path(devDir) / plannerPath.filename();

        if (!fs::exists(devPath)) continue;

        OverwriteRowsByOrder(plannerPath, devPath);
        count++;
    }

    std::string message = std::to_string(count) + "개 파일 병합 완료!";
    MessageBoxA(nullptr, message.c_str(), "완료", MB_OK | MB_ICONINFORMATION);
    return 0;
}
