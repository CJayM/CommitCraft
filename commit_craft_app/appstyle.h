#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QString>

inline QString globalStylesheet()
{
    return R"(
        /* ===== ГЛОБАЛЬНЫЕ НАСТРОЙКИ ===== */
        QMainWindow {
            background-color: #f6f8fa;
        }

        QFrame {
            border: none;
        }

        /* ===== МЕНЮ ===== */
        QMenuBar {
            background-color: #ffffff;
            border-bottom: 1px solid #d0d7de;
            padding: 2px 0px;
            font-size: 12px;
        }
        QMenuBar::item {
            padding: 2px 4px;
            border-radius: 2px;
            margin: 2px 1px;
        }
        QMenuBar::item:selected {
            background-color: #e8eaed;
        }
        QMenu {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            padding: 2px;
        }
        QMenu::item {
            padding: 2px 8px 2px 8px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #0969da;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background: #d0d7de;
            margin: 2px 4px;
        }

        /* ===== ТУЛБАР ===== */
        QToolBar {
            background-color: #ffffff;
            border-bottom: 1px solid #d0d7de;
            spacing: 2px;
            padding: 2px 2px;
        }
        QToolButton {
            border: 1px solid transparent;
            border-radius: 2px;
            padding: 2px;
            background-color: transparent;
            min-width: 14px;
            min-height: 14px;
        }
        QToolButton:hover {
            background-color: #e8eaed;
            border-color: #d0d7de;
        }
        QToolButton:pressed {
            background-color: #d0d7de;
        }
        QToolButton:checked {
            background-color: #ddf4ff;
            border-color: #0969da;
        }

        /* ===== SPLITTER ===== */
        QSplitter::handle {
            background-color: #b0b7be;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }
        QSplitter::handle:hover {
            background-color: #0969da;
        }

        /* ===== ТАБЛИЦЫ ===== */
        QTableView {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            selection-background-color: #ddf4ff;
            selection-color: #1f2328;
            font-size: 12px;
            outline: none;
        }
        QTableView::item {
            padding: 2px 4px;
            border-bottom: 1px solid #f0f2f5;
        }
        QTableView::item:hover {
            background-color: #f3f4f6;
        }
        QTableView::item:selected {
            background-color: #ddf4ff;
            color: #1f2328;
        }
        QTableView::item:selected:active {
            background-color: #ddf4ff;
        }
        QHeaderView::section {
            background-color: #f6f8fa;
            color: #656d76;
            font-weight: 600;
            font-size: 12px;
            padding: 2px 4px;
            border: 1px solid #d0d7de;
            border-bottom: 1px solid #d0d7de;
            text-transform: uppercase;
        }

        /* ===== ДЕРЕВЬЯ ===== */
        QTreeView {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            selection-background-color: #ddf4ff;
            selection-color: #1f2328;
            font-size: 12px;
            outline: none;
        }
        QTreeView::item {
            padding: 2px 4px;
            border-radius: 2px;
            min-height: 14px;
        }
        QTreeView::item:hover {
            background-color: #f3f4f6;
        }
        QTreeView::item:selected {
            background-color: #ddf4ff;
            color: #1f2328;
        }
        QTreeView::branch {
            background: transparent;
        }

        /* ===== ТЕКСТОВЫЕ ПОЛЯ ===== */
        QTextEdit, QPlainTextEdit {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            padding: 2px;
            selection-background-color: #a3d3ff;
            selection-color: #1f2328;
            font-size: 12px;
        }
        QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #0969da;
        }

        /* ===== КНОПКИ ===== */
        QPushButton {
            background-color: #f6f8fa;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            padding: 2px 4px;
            font-size: 12px;
            font-weight: 500;
            color: #1f2328;
            min-height: 14px;
        }
        QPushButton:hover {
            background-color: #e8eaed;
            border-color: #b6c3cf;
        }
        QPushButton:pressed {
            background-color: #d0d7de;
        }
        QPushButton:disabled {
            background-color: #f6f8fa;
            color: #8c959f;
            border-color: #d0d7de;
        }
        QPushButton#commitButton,
        QToolButton#commitButton {
            background-color: #0969da;
            color: #ffffff;
            border-color: #0969da;
            font-weight: 600;
            border-radius: 2px;
            padding: 2px 4px;
            min-height: 14px;
        }
        QPushButton#commitButton:hover,
        QToolButton#commitButton:hover {
            background-color: #0860ca;
        }
        QPushButton#commitButton:pressed,
        QToolButton#commitButton:pressed {
            background-color: #0550a3;
        }
        QPushButton#commitButton:disabled,
        QToolButton#commitButton:disabled {
            background-color: #92baf0;
            border-color: #92baf0;
            color: #ffffff;
        }
        QPushButton#stageSelectedButton {
            background-color: #dafbe1;
            color: #116329;
            border-color: #a6d9b7;
            font-weight: 500;
        }
        QPushButton#stageSelectedButton:hover {
            background-color: #c0ecd0;
        }
        QPushButton#revertSelectedButton {
            background-color: #ffebe9;
            color: #a0111f;
            border-color: #f8c4c0;
            font-weight: 500;
        }
        QPushButton#revertSelectedButton:hover {
            background-color: #ffd8d4;
        }

        /* ===== ЧЕКБОКС ===== */
        QCheckBox {
            spacing: 2px;
            font-size: 12px;
            color: #1f2328;
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border: 2px solid #d0d7de;
            border-radius: 2px;
            background-color: #ffffff;
        }
        QCheckBox::indicator:checked {
            background-color: #0969da;
            border-color: #0969da;
        }
        QCheckBox::indicator:hover {
            border-color: #0969da;
        }

        /* ===== ЛЕЙБЛЫ ===== */
        QLabel {
            color: #1f2328;
            font-size: 12px;
        }
        QLabel#filesLabel, QLabel#unstagedLabel, QLabel#stagedLabel,
        QLabel#commitLabel, QLabel#repoLabel, QLabel#repositoryLabel {
            font-weight: 600;
            font-size: 12px;
            color: #656d76;
            padding: 2px 4px;
            text-transform: uppercase;
        }

        /* ===== СТАТУС-БАР ===== */
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #d0d7de;
            color: #656d76;
            font-size: 12px;
            padding: 2px 4px;
        }

        /* ===== СКРОЛЛБАРЫ ===== */
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #c1c7cd;
            min-height: 30px;
            border-radius: 2px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #a0a7af;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }

        QScrollBar:horizontal {
            background: transparent;
            height: 10px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #c1c7cd;
            min-width: 30px;
            border-radius: 2px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #a0a7af;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
        }

        /* ===== ДИАЛОГИ ===== */
        QDialog {
            background-color: #ffffff;
        }

        /* ===== TOOLTIP ===== */
        QToolTip {
            background-color: #1f2328;
            color: #ffffff;
            border: none;
            border-radius: 2px;
            padding: 2px 4px;
            font-size: 12px;
        }

        /* ===== INPUT ===== */
        QLineEdit {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            padding: 2px 4px;
            font-size: 12px;
            min-height: 14px;
        }
        QLineEdit:focus {
            border-color: #0969da;
        }

        QSpinBox {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            padding: 2px;
            font-size: 13px;
            min-height: 14px;
        }
        QSpinBox:focus {
            border-color: #0969da;
        }

        QFontComboBox {
            background-color: #ffffff;
            border: 1px solid #d0d7de;
            border-radius: 2px;
            padding: 2px 4px;
            font-size: 12px;
            min-height: 14px;
        }
        QFontComboBox:focus {
            border-color: #0969da;
        }
    )";
}

#endif // APPSTYLE_H
