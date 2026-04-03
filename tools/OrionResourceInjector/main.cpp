// tools/OrionResourceInjector/main.cpp
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <winbase.h>

// .ico ファイルを読み込んで GRPICONDIR 形式に分解し、
// UpdateResource で .exe のリソースセクションに書き込む

// .ico のバイナリ構造
#pragma pack(push, 1)
struct ICONDIRENTRY {
  BYTE bWidth;
  BYTE bHeight;
  BYTE bColorCount;
  BYTE bReserved;
  WORD wPlanes;
  WORD wBitCount;
  DWORD dwBytesInRes;
  DWORD dwImageOffset;
};
struct ICONDIR {
  WORD idReserved;
  WORD idType; // 1 = ICON
  WORD idCount;
};

// RT_GROUP_ICON に書き込む GRPICONDIRENTRY
struct GRPICONDIRENTRY {
  BYTE bWidth;
  BYTE bHeight;
  BYTE bColorCount;
  BYTE bReserved;
  WORD wPlanes;
  WORD wBitCount;
  DWORD dwBytesInRes;
  WORD nID; // RT_ICON リソースID
};
struct GRPICONDIR {
  WORD idReserved;
  WORD idType;
  WORD idCount;
  // GRPICONDIRENTRY が idCount 個続く
};
#pragma pack(pop)

static std::vector<BYTE> readFile(const std::wstring &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return {};
  return {std::istreambuf_iterator<char>(f), {}};
}

int wmain(int argc, wchar_t *argv[]) {
  if (argc < 3) {
    std::wcerr << L"Usage: OrionResourceInjector <target.exe> <icon.ico>\n";
    return 1;
  }

  const std::wstring exePath = argv[1];
  const std::wstring icoPath = argv[2];
  constexpr WORD BASE_ICON_ID = 101; // IDI_APP_ICON と同じ値

  auto icoData = readFile(icoPath);
  if (icoData.empty()) {
    std::wcerr << L"Failed to read: " << icoPath << L"\n";
    return 1;
  }

  // .ico をパース
  const ICONDIR *dir = reinterpret_cast<const ICONDIR *>(icoData.data());
  const ICONDIRENTRY *entries =
      reinterpret_cast<const ICONDIRENTRY *>(icoData.data() + sizeof(ICONDIR));

  // RT_GROUP_ICON 用バッファを構築
  size_t grpSize = sizeof(GRPICONDIR) + sizeof(GRPICONDIRENTRY) * dir->idCount;
  std::vector<BYTE> grpData(grpSize);

  auto *grp = reinterpret_cast<GRPICONDIR *>(grpData.data());
  grp->idReserved = 0;
  grp->idType = 1;
  grp->idCount = dir->idCount;

  auto *grpEntries =
      reinterpret_cast<GRPICONDIRENTRY *>(grpData.data() + sizeof(GRPICONDIR));

  // UpdateResource でリソースを書き込む
  HANDLE hUpdate = BeginUpdateResourceW(exePath.c_str(), FALSE);
  if (!hUpdate) {
    std::wcerr << L"BeginUpdateResource failed: " << GetLastError() << L"\n";
    return 1;
  }

  for (WORD i = 0; i < dir->idCount; ++i) {
    const ICONDIRENTRY &e = entries[i];
    const WORD iconID = BASE_ICON_ID + i; // 101, 102, 103 ...

    // 個別アイコンデータ (RT_ICON)
    const BYTE *imgData = icoData.data() + e.dwImageOffset;

    if (!UpdateResourceW(hUpdate, (LPCWSTR)RT_ICON, MAKEINTRESOURCEW(iconID),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         const_cast<BYTE *>(imgData), e.dwBytesInRes)) {
      std::wcerr << L"UpdateResource(RT_ICON) failed at index " << i << L"\n";
      EndUpdateResourceW(hUpdate, TRUE); // 破棄
      return 1;
    }

    // GRPICONDIR エントリを埋める
    grpEntries[i].bWidth = e.bWidth;
    grpEntries[i].bHeight = e.bHeight;
    grpEntries[i].bColorCount = e.bColorCount;
    grpEntries[i].bReserved = 0;
    grpEntries[i].wPlanes = e.wPlanes;
    grpEntries[i].wBitCount = e.wBitCount;
    grpEntries[i].dwBytesInRes = e.dwBytesInRes;
    grpEntries[i].nID = iconID;
  }

  // RT_GROUP_ICON を書き込む（これがタスクバー・エクスプローラに見える）
  if (!UpdateResourceW(hUpdate, (LPCWSTR)RT_GROUP_ICON,
                       MAKEINTRESOURCEW(BASE_ICON_ID),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                       grpData.data(), static_cast<DWORD>(grpData.size()))) {
    std::wcerr << L"UpdateResource(RT_GROUP_ICON) failed\n";
    EndUpdateResourceW(hUpdate, TRUE);
    return 1;
  }

  if (!EndUpdateResourceW(hUpdate, FALSE)) {
    std::wcerr << L"EndUpdateResource failed: " << GetLastError() << L"\n";
    return 1;
  }

  std::wcout << L"[OrionResourceInjector] Icon injected: " << exePath << L"\n";
  return 0;
}