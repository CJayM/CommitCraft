#ifndef DIFFEDITOR_H
#define DIFFEDITOR_H

#include "diffpanel.h"
#include "diffhighlighter.h"
#include "hunkactionpanel.h"

#include <gitparser.h>

#include <QWidget>
#include <QFileInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
class DiffEditor;
}
QT_END_NAMESPACE

class DiffPanel;
class Highlighter;

/// Структура для синхронизированной строки diff
struct SyncedLine {
    QString leftText;   // Текст для левой панели (пустая если строка добавлена)
    QString rightText;  // Текст для правой панели (пустая если строка удалена)
    bool isRemoved;     // Строка удалена (отображается слева)
    bool isAdded;       // Строка добавлена (отображается справа)
    bool isModified;    // Строка модифицирована
    bool isContext;     // Контекстная строка (без изменений)
    bool leftIsPlaceholder;  // Левая строка — пустой заполнитель (серый фон)
    bool rightIsPlaceholder; // Правая строка — пустой заполнитель (серый фон)
    int leftLineNum;    // Номер строки в левом файле (-1 если не применяется)
    int rightLineNum;   // Номер строки в правом файле (-1 если не применяется)
    int hunkIndex;      // Индекс чанка, к которому принадлежит строка (-1 для разделителей)
    bool isHunkBoundary; // true если это первая строка нового чанка (после gap)
    bool isSeparator;    // true если это строка-разделитель между чанками
};

/// Виджет side-by-side diff с синхронным скроллом, зумом и подсветкой синтаксиса.
class DiffEditor : public QWidget
{
    Q_OBJECT

public:
    explicit DiffEditor(QWidget *parent = nullptr);
    ~DiffEditor();

    /// Установка содержимого для сравнения (из QString).
    /// \param leftContent  Содержимое левой панели (staged / HEAD)
    /// \param rightContent Содержимое правой панели (current / working tree)
    /// \param fileName     Имя файла (для определения типа подсветки синтаксиса)
    /// \param repositoryPath Путь к репозиторию (для загрузки изображений)
    void setContents(const QString &leftContent,
                     const QString &rightContent,
                     const QString &fileName,
                     const QString &repositoryPath = QString());

    /// Установка содержимого для сравнения (из сырых байтов).
    /// \param leftData  Сырые байты левой панели
    /// \param rightData Сырые байты правой панели
    /// \param fileName     Имя файла (для определения типа подсветки синтаксиса)
    /// \param repositoryPath Путь к репозиторию (для загрузки изображений)
    /// \param encoding     Кодировка для декодирования (по умолчанию UTF-8)
    void setContentsRaw(const QByteArray &leftData,
                        const QByteArray &rightData,
                        const QString &fileName,
                        const QString &repositoryPath = QString(),
                        const QString &encoding = "UTF-8");

    /// Очистить содержимое
    void clear();

    /// Применить diff-данные и перестроить side-by-side view
    void applyDiffData(const QList<Hunk> &hunks);

    /// Установить режим diff (staged/unstaged) для HunkActionPanel
    void setDiffMode(HunkActionPanel::DiffMode mode);

    /// Применить настройки шрифта к обеим панелям
    void applyFontSettings(const QString &fontFamily, int fontSize);

    /// Проверка, является ли файл графическим
    bool isImageFile() const;

    /// Проверка, нужна ли подсветка синтаксиса
    bool isSyntaxFile() const;

    /// Обновить настройки расширений из QSettings
    void updateFileTypeSettings();

    /// Установить флаг «файл новый (untracked)» — показать кнопку Stage для всего файла
    void setFileIsNew(bool isNew);

    /// Показать сообщение о слишком большом файле (вместо diff)
    void showLargeFileMessage(const QString &fileName, qint64 fileSize);

signals:
    void hunkNavigated(int hunkIndex);
    void stageSelectedPatch(const QString &fileName, const QString &patch);
    void revertSelectedPatch(const QString &fileName, const QString &patch);
    /// Запрос на добавление нового (untracked) файла целиком
    void stageNewFileRequested(const QString &fileName);

public slots:
    /// Навигация к следующему ханку. Возвращает false если достигнут конец списка.
    bool navigateToNextHunk();
    /// Навигация к предыдущему ханку. Возвращает false если достигнуто начало списка.
    bool navigateToPrevHunk();
    /// Переход к первому ханку (начало файла)
    void navigateToFirstHunk();
    /// Переход к последнему ханку (конец файла)
    void navigateToLastHunk();
    
    int currentHunkIndex() const { return m_currentHunkIndex; }
    int hunkCount() const { return m_hunkPositions.size(); }
    
    /// Проверка границ навигации
    bool isAtFirstHunk() const { return m_currentHunkIndex <= 0; }
    bool isAtLastHunk() const { return m_hunkPositions.isEmpty() || m_currentHunkIndex >= m_hunkPositions.size() - 1; }

protected:
    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void synchronizeScrollLeftToRight(int value);
    void synchronizeScrollRightToLeft(int value);
    void synchronizeZoom(int zoom);
    void onStageSelectedClicked();
    void onRevertSelectedClicked();
    void onStageHunkClicked(int hunkIndex);
    void onRevertHunkClicked(int hunkIndex);
    void onEncodingChanged(int index);

private:
    QStringList getSelectedLines() const;
    QString buildPatchForHunks(const QString &fileName, const QList<int> &hunkIndexes) const;
    QList<int> getSelectedHunkIndexes() const;
    QList<Hunk> m_currentHunks;
    QVector<SyncedLine> m_syncedLines;

    /// Построить синхронизированные строки из hunks и полного содержимого
    QVector<SyncedLine> buildSyncedLines(const QList<Hunk> &hunks,
                                         const QStringList &leftLines,
                                         const QStringList &rightLines);

    /// Заполнить панели синхронизированным содержимым
    void populatePanels(const QVector<SyncedLine> &syncedLines);

    /// Прокрутить к позиции ханка
    void scrollToHunk(int hunkIndex);

    /// Применить подсветку синтаксиса по типу файла
    void applySyntaxHighlighting(const QString &fileName);

    /// Проверить тип файла и вернуть true если это графический файл
    bool checkFileType(const QString &fileName);

    /// Обновить видимость кнопок partial staging
    void updateButtonsVisibility();

    Ui::DiffEditor *ui;
    bool m_isImageFile;         // Флаг графического файла
    QStringList m_imageExtensions;  // Расширения графических файлов
    QStringList m_syntaxExtensions; // Расширения для подсветки синтаксиса
    QString m_repositoryPath;   // Путь к репозиторию для загрузки изображений

    // Навигация по ханкам
    QVector<int> m_hunkPositions; // Индексы первых строк ханков в syncedLines
    int m_currentHunkIndex;

    // Флаг для предотвращения рекурсии при синхронизации скролла
    bool m_syncingScroll;

    // Полное содержимое файлов (для side-by-side diff)
    QString m_leftFullContent;
    QString m_rightFullContent;
    QByteArray m_leftRawData;   // Сырые байты для перекодировки
    QByteArray m_rightRawData;  // Сырые байты для перекодировки
    QString m_currentEncoding;  // Текущая кодировка
    QString m_fileName;
    bool m_fileIsNew = false;  // true если файл untracked (новый)
};

#endif // DIFFEDITOR_H
