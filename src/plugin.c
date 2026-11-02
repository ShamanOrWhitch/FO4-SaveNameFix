/* F4SE plugin. Loaded by f4se_loader, not by hand.
   Saves stay in Documents\My Games\Fallout4\Saves.
   The plugin shortens Exitsave0_00000000SM....fos to Exitsave0.fos
   and cuts the in-memory format string so new saves are written short. */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>

typedef uint32_t UInt32;
typedef int32_t HRESULT;
typedef void* HANDLE;
typedef int BOOL;

#define WINAPI __stdcall
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define MOVEFILE_REPLACE_EXISTING 0x1
#define MAX_PATH 260

typedef struct {
	unsigned long Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
} GUID;

typedef struct {
	unsigned long LowPart;
	long HighPart;
} FILETIME;

typedef struct {
	unsigned long dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	unsigned long nFileSizeHigh;
	unsigned long nFileSizeLow;
	unsigned long dwReserved0;
	unsigned long dwReserved1;
	wchar_t cFileName[MAX_PATH];
	wchar_t cAlternateFileName[14];
} WIN32_FIND_DATAW;

typedef struct {
	unsigned long dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	unsigned long nFileSizeHigh;
	unsigned long nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;

__declspec(dllimport) HANDLE WINAPI FindFirstFileW(const wchar_t*, WIN32_FIND_DATAW*);
__declspec(dllimport) BOOL WINAPI FindNextFileW(HANDLE, WIN32_FIND_DATAW*);
__declspec(dllimport) BOOL WINAPI FindClose(HANDLE);
__declspec(dllimport) BOOL WINAPI MoveFileExW(const wchar_t*, const wchar_t*, unsigned long);
__declspec(dllimport) BOOL WINAPI GetFileAttributesExW(const wchar_t*, int, void*);
__declspec(dllimport) long WINAPI CompareFileTime(const FILETIME*, const FILETIME*);
__declspec(dllimport) BOOL WINAPI CreateDirectoryW(const wchar_t*, void*);
__declspec(dllimport) HANDLE WINAPI CreateFileW(const wchar_t*, unsigned long, unsigned long, void*, unsigned long, unsigned long, HANDLE);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE, const void*, unsigned long, unsigned long*, void*);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
__declspec(dllimport) void WINAPI Sleep(unsigned long);
__declspec(dllimport) HANDLE WINAPI GetModuleHandleW(const wchar_t*);
__declspec(dllimport) BOOL WINAPI VirtualProtect(void*, size_t, unsigned long, unsigned long*);
__declspec(dllimport) HANDLE WINAPI CreateThread(void*, size_t, unsigned long (WINAPI*)(void*), void*, unsigned long, unsigned long*);
__declspec(dllimport) int WINAPI WideCharToMultiByte(unsigned int, unsigned long, const wchar_t*, int, char*, int, const char*, int*);
__declspec(dllimport) int WINAPI MultiByteToWideChar(unsigned int, unsigned long, const char*, int, wchar_t*, int);
__declspec(dllimport) HRESULT WINAPI SHGetKnownFolderPath(const GUID*, unsigned long, HANDLE, wchar_t**);
__declspec(dllimport) void WINAPI CoTaskMemFree(void*);

#define GENERIC_WRITE 0x40000000
#define FILE_APPEND_DATA 4
#define FILE_SHARE_READ 1
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0x80
#define PAGE_READWRITE 0x04
#define GetFileExInfoStandard 0

enum { kInterface_Messaging = 1 };
enum { kMessage_PostSaveGame = 5, kMessage_InputLoaded = 7 };

typedef UInt32 PluginHandle;

struct F4SEInterface {
	UInt32 f4seVersion;
	UInt32 runtimeVersion;
	UInt32 editorVersion;
	UInt32 isEditor;
	void* (*QueryInterface)(UInt32 id);
	PluginHandle (*GetPluginHandle)(void);
	UInt32 (*GetReleaseIndex)(void);
	const void* (*GetPluginInfo)(const char* name);
	const char* (*GetSaveFolderName)(void);
};

struct F4SEMessage {
	const char* sender;
	UInt32 type;
	UInt32 dataLen;
	void* data;
};

struct F4SEMessagingInterface {
	UInt32 interfaceVersion;
	int (*RegisterListener)(PluginHandle listener, const char* sender, void (*handler)(struct F4SEMessage*));
	int (*Dispatch)(PluginHandle, UInt32, void*, UInt32, const char*);
	void* (*GetEventDispatcher)(UInt32);
};

struct F4SEPluginVersionData {
	UInt32 dataVersion;
	UInt32 pluginVersion;
	char name[256];
	char author[256];
	UInt32 addressIndependence;
	UInt32 structureIndependence;
	UInt32 compatibleVersions[16];
	UInt32 seVersionRequired;
	UInt32 reservedNonBreaking;
	UInt32 reservedBreaking;
	unsigned char reserved[512];
};

enum {
	kAddressIndependence_Signatures = 1 << 0,
	kStructureIndependence_NoStructs = 1 << 0
};

/* 0.7.1, when GetSaveFolderName was added. */
enum { kF4SE_0_7_1 = 0x00070010 };

static struct F4SEInterface* g_f4se;
static volatile int g_busy;

static int is_hex(wchar_t c) {
	return (c >= L'0' && c <= L'9') || (c >= L'A' && c <= L'F') || (c >= L'a' && c <= L'f');
}

/* Exitsave0_00000000SM4A4168436F7265_Commonwealth_....f4se -> Exitsave0.f4se */
static int short_name(const wchar_t* name, wchar_t* out, int cap) {
	const wchar_t* dot = NULL;
	const wchar_t* p;
	const wchar_t* ext;
	int i, n, mark;
	if (!name || !name[0]) return 0;
	for (p = name; *p; ++p) {
		if (*p == L'.') dot = p;
	}
	if (!dot || dot == name) return 0;
	ext = dot + 1;
	if (_wcsicmp(ext, L"fos") != 0 && _wcsicmp(ext, L"f4se") != 0) return 0;
	n = (int)(dot - name);
	mark = -1;
	for (i = 0; i + 11 <= n; ++i) {
		int h, ok;
		if (name[i] != L'_') continue;
		ok = 1;
		for (h = 1; h <= 8; ++h) {
			if (!is_hex(name[i + h])) { ok = 0; break; }
		}
		if (!ok) continue;
		if ((name[i + 9] == L'S' || name[i + 9] == L's') &&
		    (name[i + 10] == L'M' || name[i + 10] == L'm')) {
			mark = i;
			break;
		}
	}
	if (mark <= 0) return 0;
	if (mark + 1 + (int)wcslen(ext) + 1 >= cap) return 0;
	memcpy(out, name, (size_t)mark * sizeof(wchar_t));
	out[mark] = L'.';
	wcscpy(out + mark + 1, ext);
	return 1;
}

static wchar_t g_saves[1024];
static wchar_t g_logpath[1024];

static void log_line(const char* text) {
	HANDLE f;
	unsigned long wrote;
	if (!g_logpath[0]) return;
	f = CreateFileW(g_logpath, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) return;
	WriteFile(f, text, (unsigned long)strlen(text), &wrote, NULL);
	WriteFile(f, "\r\n", 2, &wrote, NULL);
	CloseHandle(f);
}

static void log_rename(const wchar_t* from, const wchar_t* to) {
	char buf[1200];
	char a[400], b[400];
	WideCharToMultiByte(65001, 0, from, -1, a, (int)sizeof a, NULL, NULL);
	WideCharToMultiByte(65001, 0, to, -1, b, (int)sizeof b, NULL, NULL);
	_snprintf(buf, sizeof buf, "rename %s -> %s", a, b);
	buf[sizeof buf - 1] = 0;
	log_line(buf);
}

static void fix_saves(void) {
	wchar_t pattern[1100];
	wchar_t src[1100];
	wchar_t dst[1100];
	wchar_t parked[1100];
	wchar_t shortn[MAX_PATH];
	WIN32_FIND_DATAW fd;
	HANDLE h;
	if (!g_saves[0]) return;
	if (g_busy) return;
	g_busy = 1;
	_snwprintf(pattern, 1100, L"%s\\*", g_saves);
	h = FindFirstFileW(pattern, &fd);
	if (h == INVALID_HANDLE_VALUE) {
		g_busy = 0;
		return;
	}
	do {
		WIN32_FILE_ATTRIBUTE_DATA srcInfo, dstInfo;
		int haveDst;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		if (!short_name(fd.cFileName, shortn, MAX_PATH)) continue;
		_snwprintf(src, 1100, L"%s\\%s", g_saves, fd.cFileName);
		_snwprintf(dst, 1100, L"%s\\%s", g_saves, shortn);
		haveDst = GetFileAttributesExW(dst, GetFileExInfoStandard, &dstInfo);
		GetFileAttributesExW(src, GetFileExInfoStandard, &srcInfo);
		if (haveDst && CompareFileTime(&srcInfo.ftLastWriteTime, &dstInfo.ftLastWriteTime) <= 0) {
			_snwprintf(parked, 1100, L"%s\\%s", g_saves, shortn);
			/* keep the newer short file; drop SM from the old long name */
			{
				wchar_t* dot = wcsrchr(shortn, L'.');
				if (!dot) continue;
				*dot = 0;
				_snwprintf(parked, 1100, L"%s\\%s_old.%s", g_saves, shortn, dot + 1);
				*dot = L'.';
			}
			if (MoveFileExW(src, parked, MOVEFILE_REPLACE_EXISTING))
				log_rename(fd.cFileName, parked);
			continue;
		}
		if (MoveFileExW(src, dst, MOVEFILE_REPLACE_EXISTING))
			log_rename(fd.cFileName, shortn);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
	g_busy = 0;
}

static void patch_format(void) {
	unsigned char* base;
	unsigned char* dos;
	long lfanew;
	unsigned char* nt;
	unsigned long size, i;
	const char sig[] = "%s%u_%s%s%s_%s_%s_%s_%u_%u";
	const unsigned long siglen = sizeof sig - 1;
	int hits = 0;
	unsigned char* at = NULL;
	base = (unsigned char*)GetModuleHandleW(NULL);
	if (!base) return;
	dos = base;
	if (dos[0] != 'M' || dos[1] != 'Z') return;
	lfanew = *(long*)(dos + 0x3C);
	nt = base + lfanew;
	if (nt[0] != 'P' || nt[1] != 'E') return;
	size = *(unsigned long*)(nt + 24 + 56); /* OptionalHeader.SizeOfImage */
	if (size < siglen || size > 0x08000000) return;
	for (i = 0; i + siglen < size; ++i) {
		if (memcmp(base + i, sig, siglen) == 0) {
			hits++;
			at = base + i;
		}
	}
	if (hits != 1 || !at) {
		log_line("format string not unique, exe left unchanged");
		return;
	}
	if (at[4] == 0) {
		log_line("format already short");
		return;
	}
	if (at[4] == '_') {
		unsigned long old = 0;
		if (VirtualProtect(at, 8, PAGE_READWRITE, &old)) {
			at[4] = 0;
			VirtualProtect(at, 8, old, &old);
			log_line("format cut to %s%u");
		} else {
			log_line("VirtualProtect failed");
		}
	}
}

static int build_saves_path(void) {
	GUID documents = { 0xFDD39AD0, 0x238F, 0x46AF, { 0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7 } };
	wchar_t* docs = NULL;
	wchar_t folder[64];
	const char* raw = "Fallout4";
	HRESULT hr;
	folder[0] = 0;
	if (g_f4se && g_f4se->f4seVersion >= kF4SE_0_7_1 && g_f4se->GetSaveFolderName) {
		const char* got = g_f4se->GetSaveFolderName();
		if (got && got[0]) raw = got;
	}
	MultiByteToWideChar(65001, 0, raw, -1, folder, 64);
	hr = SHGetKnownFolderPath(&documents, 0, NULL, &docs);
	if (hr < 0 || !docs) return 0;
	_snwprintf(g_saves, 1024, L"%s\\My Games\\%s\\Saves", docs, folder);
	_snwprintf(g_logpath, 1024, L"%s\\My Games\\%s\\F4SE", docs, folder);
	CreateDirectoryW(g_logpath, NULL);
	_snwprintf(g_logpath, 1024, L"%s\\My Games\\%s\\F4SE\\FO4SaveNameFix.log", docs, folder);
	CoTaskMemFree(docs);
	return g_saves[0] != 0;
}

static unsigned long WINAPI watch_thread(void* unused) {
	(void)unused;
	for (;;) {
		fix_saves();
		Sleep(1000);
	}
	return 0;
}

static void on_f4se(struct F4SEMessage* msg) {
	int i;
	if (!msg) return;
	if (msg->type == kMessage_InputLoaded || msg->type == kMessage_PostSaveGame) {
		for (i = 0; i < 20; ++i) {
			fix_saves();
			if (msg->type != kMessage_PostSaveGame) break;
			Sleep(100);
		}
	}
}

static void go(void) {
	if (!build_saves_path()) {
		log_line("no saves path");
		return;
	}
	log_line("FO4SaveNameFix loaded");
	patch_format();
	fix_saves();
	CreateThread(NULL, 0, watch_thread, NULL, 0, NULL);
}

__declspec(dllexport) struct F4SEPluginVersionData F4SEPlugin_Version = {
	1,
	1,
	"FO4SaveNameFix",
	"ShamanOrWhitch",
	kAddressIndependence_Signatures,
	kStructureIndependence_NoStructs,
	{ 0 },
	0,
	0,
	0,
	{ 0 }
};

__declspec(dllexport) int F4SEPlugin_Preload(const struct F4SEInterface* f4se) {
	g_f4se = (struct F4SEInterface*)f4se;
	go();
	return 1;
}

__declspec(dllexport) int F4SEPlugin_Load(const struct F4SEInterface* f4se) {
	struct F4SEMessagingInterface* msg;
	PluginHandle handle;
	g_f4se = (struct F4SEInterface*)f4se;
	if (!g_saves[0]) go();
	if (!f4se || !f4se->QueryInterface || !f4se->GetPluginHandle) return 1;
	handle = f4se->GetPluginHandle();
	msg = (struct F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);
	if (msg && msg->RegisterListener)
		msg->RegisterListener(handle, "F4SE", on_f4se);
	return 1;
}
