#include "commititemdelegate.h"
#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>
#include <QFontMetrics>

// Цвет ветки по индексу колонки (должен совпадать с моделью)
static QColor branchColorForColumn(int col)
{
    static const QVector<QColor> colors = {
        QColor(230, 57, 70),    // красный
        QColor(46, 204, 113),   // зеленый
        QColor(52, 152, 219),   // синий
        QColor(155, 89, 182),   // фиолетовый
        QColor(241, 196, 15),   // желтый
        QColor(230, 126, 34),   // оранжевый
        QColor(26, 188, 156),   // бирюзовый
        QColor(241, 128, 23),   // темно-оранжевый
    };
    return colors[col % colors.size()];
}
CommitItemDelegate::CommitItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void CommitItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Получаем данные коммита из модели
    QVariant data = index.data(Qt::UserRole);
    if (!data.canConvert<CommitData>()) {
        // Fallback к стандартной отрисовке
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    
    CommitData commit = data.value<CommitData>();
    
    // Рисуем фон выделения
    QStyleOptionViewItem opt = option;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);
    
    // Разная отрисовка для разных колонок
    if (index.column() == 0) {
        paintGraphColumn(painter, option, commit);
    } else if (index.column() == 1) {
        paintMessageColumn(painter, option, commit);
    }
}

QSize CommitItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(36); // Фиксированная высота для строки коммита
    return size;
}

void CommitItemDelegate::paintGraphColumn(QPainter *painter, const QStyleOptionViewItem &option, const CommitData &commit) const
{
    QRect rect = option.rect;

    // Параметры графа
    const int dotRadius = 6;
    const int lineHeight = 3;
    const int columnSpacing = 20;
    const int leftMargin = 8;

    int centerY = rect.center().y();
    int dotX = leftMargin + commit.graphColumn * columnSpacing + dotRadius;
    int dotY = centerY;

    painter->setRenderHint(QPainter::Antialiasing, true);

    // Определяем, является ли этот коммит созданием новой ветки
    int firstParentCol = commit.parentColumns.isEmpty() ? -1 : commit.parentColumns[0];
    bool isBranchCreation = (!commit.parents.isEmpty()
                             && firstParentCol >= 0
                             && firstParentCol != commit.graphColumn);

    // 1. Вертикальные линии для ВСЕХ активных колонок
    for (int col : commit.activeColumns) {
        int colX = leftMargin + col * columnSpacing + dotRadius;
        painter->setPen(QPen(branchColorForColumn(col), lineHeight));

        if (col == commit.graphColumn && isBranchCreation) {
            // Новая ветка — линия только ВВЕРХ (ниже ничего нет)
            painter->drawLine(colX, rect.top(), colX, dotY - dotRadius);
        } else {
            // Остальные колонки и обычные коммиты — полная вертикаль
            painter->drawLine(colX, rect.top(), colX, rect.bottom());
        }
    }

    // 2. Диагональ ответвления (родитель в другой колонке — создание ветки)
    if (isBranchCreation) {
        int parentDotX = leftMargin + firstParentCol * columnSpacing + dotRadius;
        // От нашей точки ВНИЗ-ВЛЕВО к колонке родителя
        painter->setPen(QPen(branchColorForColumn(firstParentCol), lineHeight));
        painter->drawLine(dotX, dotY, parentDotX, rect.bottom());
    }

    // 3. Диагонали слияния (второй и последующие родители)
    for (int p = 1; p < commit.parents.size(); ++p) {
        int parentCol = (p < commit.parentColumns.size())
            ? commit.parentColumns[p] : -1;
        if (parentCol < 0)
            continue;
        int parentDotX = leftMargin + parentCol * columnSpacing + dotRadius;
        // От точки merge ВНИЗ к колонке второго родителя
        painter->setPen(QPen(branchColorForColumn(parentCol), lineHeight));
        painter->drawLine(dotX, dotY, parentDotX, rect.bottom());
    }

    // 4. Точка коммита
    painter->setBrush(commit.branchColor);
    painter->setPen(commit.branchColor);
    painter->drawEllipse(QPoint(dotX, dotY), dotRadius, dotRadius);

    // 5. Хэш коммита
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    QFontMetrics fmBold(boldFont);
    int hashX = leftMargin + 100;
    painter->setFont(boldFont);
    painter->setPen(commit.branchColor);
    painter->drawText(hashX, rect.top() + fmBold.ascent(), commit.shortHash);
}

void CommitItemDelegate::paintMessageColumn(QPainter *painter, const QStyleOptionViewItem &option, const CommitData &commit) const
{
    painter->save();
    painter->setClipRect(option.rect);
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Устанавливаем цвета текста
    QColor textColor = option.palette.color(QPalette::Text);
    if (option.state & QStyle::State_Selected) {
        textColor = option.palette.color(QPalette::HighlightedText);
    }

    // Получаем rect для рисования
    QRect rect = option.rect;
    rect.adjust(5, 2, -5, -2);

    // Шрифты
    QFont font = painter->font();
    QFontMetrics fm(font);

    int baseY = rect.top();

    // --- Строка 1: сообщение + refs (inline) ---
    int messageWidth = rect.width();
    int refsTotalWidth = 0;

    // Сначала вычисляем ширину всех refs
    if (!commit.refs.isEmpty()) {
        for (const QString &ref : commit.refs) {
            int refWidth = fm.horizontalAdvance(ref) + 12;
            refsTotalWidth += refWidth + 4; // +4 для промежутка между бейджами
        }
        if (refsTotalWidth > 0)
            refsTotalWidth -= 4; // убираем последний промежуток
        messageWidth = rect.width() - refsTotalWidth - 8;
    }

    // Рисуем сообщение коммита (слева, elided с учётом места под refs)
    painter->setFont(font);
    painter->setPen(textColor);
    QString elidedMessage = fm.elidedText(commit.message, Qt::ElideRight,
                                          qMax(messageWidth, 20));
    painter->drawText(rect.left(), baseY + fm.ascent(), elidedMessage);

    // Рисуем refs (ветки, теги) справа на той же строке
    if (!commit.refs.isEmpty()) {
        int refsX = rect.right();
        for (const QString &ref : commit.refs) {
            bool isBranch = ref.contains("HEAD") || !ref.contains("tag");
            QColor refColor = isBranch ? QColor(230, 57, 70) : QColor(155, 89, 182);

            int refWidth = fm.horizontalAdvance(ref) + 12;
            int refHeight = fm.height() + 6;
            int refY = baseY - 1; // чуть выше базовой линии для центрирования
            QRect refRect(refsX - refWidth, refY, refWidth, refHeight);

            painter->setPen(refColor);
            painter->setBrush(QColor(refColor.red(), refColor.green(), refColor.blue(), 30));
            painter->drawRoundedRect(refRect, 3, 3);

            painter->setPen(refColor);
            painter->setFont(font);
            painter->drawText(refRect.adjusted(6, 0, -6, 0), Qt::AlignVCenter, ref);

            refsX -= refWidth + 4;
        }
    }

    // --- Строка 2: дата и автор ---
    int messageHeight = fm.height() + 4;
    painter->setPen(Qt::gray);
    QString dateAuthor = commit.author + ", " + commit.date;
    QString elidedDateAuthor = fm.elidedText(dateAuthor, Qt::ElideRight, rect.width());
    painter->drawText(rect.left(), baseY + messageHeight + fm.ascent(), elidedDateAuthor);

    painter->restore();
}
