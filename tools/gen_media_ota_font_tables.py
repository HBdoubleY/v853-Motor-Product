#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate 字体库/*Char* tables: lines are '字号 <representative text>' plus MERGED unique chars."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
I18N = ROOT / "lvgl-gui/ui/generated/i18n/i18n.h"
OUT_DIR = ROOT / "字体库"

# Keys whose translated strings are rendered with each project font (see MediaLibrary/*.c OTA/*.c)
HYBY_40 = [
    "Front", "Rear", "Urgent", "Front_Photo", "Rear_Photo",
    "Front_Videos", "Rear_Videos", "Urgent_Videos", "SDCardTitle",
]
HYBY_36 = ["SDCardFormat", "No"]  # format dialog: No btn + format button label
HYBY_30 = [
    "Yes", "Format_confirm", "Format_success", "Format_failed",
    "SDCardNotFound", "SDCardInsertTip", "SDCardUsed", "SDCardFree",
    "No_photo_found", "check_contents", "Supported_formats",
    "Found", "photos", "Delete_photo_confirm", "Record",
    "No_video_found", "Delete_video_confirm",
    "ota_download_failed", "ota_download_version_file_failed", "ota_invalid_version",
    "ota_no_need_upgrade", "ota_up_to_date", "ota_update_available",
    "ota_download_completed", "ota_file_download_err",
    "wifi_scanning", "wifi_obtaining_ip",
]
OTA_42 = ["ota_title", "ota_network_unavailable"]
OTA_36 = ["ota_downloading"]
OTA_32 = ["ota_check_updates", "ota_confirm_upgrade", "ota_update_available"]
OTA_28 = [
    "ota_system_upgrade", "ota_file_download_err",
    "ota_download_failed", "ota_download_version_file_failed", "ota_invalid_version",
    "ota_no_need_upgrade", "ota_up_to_date", "ota_download_completed",
]
OTA_26 = ["ota_note", "ota_yes", "ota_no", "ota_update_question"]
OTA_20 = ["ota_version_info", "ota_current_version"]
WIFI_42 = ["wifi_title"]
WIFI_28 = ["wifi_connected", "wifi_failed"]
WIFI_24 = ["wifi_connecting"]
WIFI_20 = ["wifi_ok"]
WIFI_18 = [
    "wifi_check_password_or_network", "wifi_connected_success",
    "wifi_status_scanning", "wifi_status_connecting",
]
WIFI_16 = ["wifi_connect", "wifi_cancel"]

def extract_language_block(text: str, name: str) -> str:
    mark = f"static const language_data_t {name} = {{"
    idx = text.find(mark)
    if idx < 0:
        return ""
    i = text.find("{", idx)
    depth = 0
    start = i
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : i]
        i += 1
    return ""


def parse_c_string_literals(block: str) -> dict:
    """Parse .key = "a" "b" ... C string initializers; UTF-8 in source preserved."""
    out = {}
    i = 0
    n = len(block)
    while i < n:
        m = re.match(r"\.(\w+)\s*=\s*", block[i:])
        if not m:
            i += 1
            continue
        key = m.group(1)
        i += m.end()
        while i < n and block[i] in " \t\n":
            i += 1
        if i >= n or block[i] != '"':
            nl = block.find("\n    .", i)
            if nl < 0:
                break
            i = nl + 1
            continue
        parts = []
        while i < n and block[i] == '"':
            i += 1
            sb = []
            while i < n:
                c = block[i]
                if c == '"':
                    i += 1
                    break
                if c == "\\" and i + 1 < n:
                    e = block[i + 1]
                    if e == "n":
                        sb.append("\n")
                        i += 2
                        continue
                    if e == "t":
                        sb.append("\t")
                        i += 2
                        continue
                    if e == '"':
                        sb.append('"')
                        i += 2
                        continue
                    if e == "\\":
                        sb.append("\\")
                        i += 2
                        continue
                    sb.append(c)
                    i += 1
                    continue
                sb.append(c)
                i += 1
            parts.append("".join(sb))
            while i < n and block[i] in " \t\n":
                i += 1
        out[key] = "".join(parts)
        while i < n and block[i] in " \t\n,":
            i += 1
    return out


def parse_i18n_simple(text: str) -> tuple[dict, dict]:
    zh = parse_c_string_literals(extract_language_block(text, "language_chinese"))
    en = parse_c_string_literals(extract_language_block(text, "language_english"))
    return zh, en


def merged_chars(keys: list[str], zh: dict, en: dict) -> str:
    s = []
    for k in keys:
        for d in (zh, en):
            v = d.get(k)
            if v:
                s.append(v)
    # common UI digits/symbols for format strings
    s.append("0123456789%:-./ \nGB")
    chars = []
    seen = set()
    for ch in "".join(s):
        if ch not in seen:
            seen.add(ch)
            chars.append(ch)
    return "".join(chars)


def write_table(path: Path, size: int, label: str, keys: list[str], zh: dict, en: dict, examples_zh: list[str]):
    mc = merged_chars(keys, zh, en)
    lines = [
        f"# {label} — 对应 v853 工程内字号 {size}，用于 lv_font_conv / 字模工具选字",
        f"# 下列每行：{size} <该界面典型文案>（可按行单独制字模，或仅用底部 MERGED）",
    ]
    for ex in examples_zh:
        lines.append(f"{size} {ex}")
    lines.append(f"# MERGED_UNIQUE (简中+英文文案去重合并，含数字与 GB/%% 等，多语言全量请用 i18n.h 自行合并)")
    lines.append(f"{size} {mc}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    text = I18N.read_text(encoding="utf-8")
    zh, en = parse_i18n_simple(text)

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    write_table(
        OUT_DIR / "MediaLibraryChar30",
        30,
        "媒体库 v853Font_hyby_30",
        HYBY_30,
        zh,
        en,
        [
            "未检测到TF卡",
            "请插入TF卡后重试",
            "已使用: 0.00 GB / 0.00 GB",
            "可用: 0.00 GB",
            "是否格式存储空间?",
            "格式化成功",
            "未找到视频",
            "确认要删除这个视频吗?",
        ],
    )
    write_table(
        OUT_DIR / "MediaLibraryChar36",
        36,
        "媒体库 v853Font_hyby_36（格式化按钮、格式化对话框「否」）",
        HYBY_36,
        zh,
        en,
        ["格式化存储", "否"],
    )
    write_table(
        OUT_DIR / "MediaLibraryChar40",
        40,
        "媒体库 v853Font_hyby_40（Tab 与存储标题）",
        HYBY_40,
        zh,
        en,
        ["存储空间", "前置录像", "紧急录像", "前置视频"],
    )

    specs = [
        ("OTA_Char20", 20, "OTA v853Font_OTA_20", OTA_20, ["当前版本: v1.0.0"]),
        ("OTA_Char26", 26, "OTA v853Font_OTA_26", OTA_26, ["是", "否", "说明多行"]),
        ("OTA_Char28", 28, "OTA v853Font_OTA_28", OTA_28, ["系统即将重新启动。", "下载失败"]),
        ("OTA_Char32", 32, "OTA v853Font_OTA_32", OTA_32, ["检查更新", "是否确认升级?"]),
        ("OTA_Char36", 36, "OTA v853Font_OTA_36", OTA_36, ["下载中..."]),
        ("OTA_Char42", 42, "OTA v853Font_OTA_42", OTA_42, ["OTA升级", "网络不可用"]),
        ("WIFI_Char16", 16, "WiFi v853Font_WIFI_16", WIFI_16, ["连接", "取消"]),
        ("WIFI_Char18", 18, "WiFi v853Font_WIFI_18", WIFI_18, ["请检查密码或网络", "扫描中... (1/20)"]),
        ("WIFI_Char20", 20, "WiFi v853Font_WIFI_20", WIFI_20, ["确定"]),
        ("WIFI_Char24", 24, "WiFi v853Font_WIFI_24", WIFI_24, ["正在连接WiFi"]),
        ("WIFI_Char28", 28, "WiFi v853Font_WIFI_28", WIFI_28, ["连接成功", "连接失败"]),
        ("WIFI_Char42", 42, "WiFi v853Font_WIFI_42", WIFI_42, ["WiFi"]),
    ]
    for fname, size, lab, keys, ex in specs:
        write_table(OUT_DIR / fname, size, lab, keys, zh, en, ex)

    print("written to", OUT_DIR)


if __name__ == "__main__":
    main()
