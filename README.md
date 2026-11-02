# FO4-SaveNameFix

Плагин для F4SE. Батник запускать не нужно: игру поднимает `f4se_loader.exe`, он сам грузит DLL.

Сейвы лежат в `Документы\My Games\Fallout4\Saves`. Игра лежит в другой папке. Плагин стоит рядом с игрой и оттуда правит файлы в Документах.

Файл со скрина

`Exitsave0_00000000SM4A4168436F7265_Commonwealth_000311_20260825000949_4_2.f4se`

становится `Exitsave0.f4se`. То же для `.fos`, `Autosave2`, `Autosave3`.

## Куда класть

1. Скачай [FO4SaveNameFix.zip](https://github.com/ShamanOrWhitch/FO4-SaveNameFix/releases/download/v1/FO4SaveNameFix.zip).
2. Открой папку, где лежит `Fallout4.exe`.
3. Скопируй туда папку `Data` из архива. Должно получиться `Data\F4SE\Plugins\FO4SaveNameFix.dll`.
4. Запускай игру через `f4se_loader.exe` в той же папке, не через лаунчер Bethesda.

После входа в главном меню хвост `SM` у сейвов уже срезан. Новые сохранения пишутся как `Exitsave0.fos` и `Autosave1.fos`.

Лог: `Документы\My Games\Fallout4\F4SE\FO4SaveNameFix.log`.

Нужен F4SE 0.7.x под твою версию игры (1.11.240). Радио Jah Bless это не трогает.
