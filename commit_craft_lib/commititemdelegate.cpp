#include "commititemdelegate.h"
#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>
#include <QFontMetrics>

CommitItemDelegate::CommitItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void CommitItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Получаем данные коммита из модели
    QVariant data = index.data(Qt::UserRole);
    if (data.canConvert<CommitData>()) {
        CommitData commit = data.value<CommitData>();
        
        // Рисуем фон выделения
        QStyleOptionViewItem opt = option;
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);
        
        // Рисуем граф в первой колонке
        if (index.column() == 0) {
            paintGraph(painter, option, commit);
        }
        
        // Рисуем содержимое коммита
        paintCommitItem(painter, option, commit);
    } else {
        // Fallback к стандартной отрисовке
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize CommitItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(32); // Фиксированная высота для строки коммита
    return size;
}

void CommitItemDelegate::paintGraph(QPainter *painter, const QStyleOptionViewItem &option, const CommitData &commit) const
{
    QRect rect = option.rect;
    
    // Параметры графа
    const int graphWidth = 100;
    const int dotRadius = 5;
    const int lineHeight = 3;
    const int columnSpacing = 18;
    
    // Центр коммита по вертикали
    int centerY = rect.center().y();
    
    // Позиция точки коммита на основе graphColumn
    int dotX = 10 + commit.graphColumn * columnSpacing;
    int dotY = centerY;
    
    // Рисуем вертикальную линию вниз (к следующему коммиту в этой ветке)
    painter->setPen(QPen(commit.branchColor, lineHeight));
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawLine(dotX, dotY, dotX, rect.bottom());
    
    // Рисуем точку коммита
    painter->setBrush(commit.branchColor);
    painter->setPen(commit.branchColor);
    painter->drawEllipse(QPoint(dotX, dotY), dotRadius, dotRadius);
    
    // Если есть родители, рисуем линии к ним
    if (!commit.parents.isEmpty()) {
        // Для merge-коммитов с несколькими родителями
        for (int i = 0; i < commit.parents.size() && i < 3; ++i) {
            // В реальной реализации нужно знать позицию родителя
            // Здесь упрощенно рисуем линии вверх
            int parentOffset = (i == 0) ? 0 : ((i % 2 == 0) ? (i/2 + 1) * columnSpacing : -(i/2 + 1) * columnSpacing);
            int parentX = dotX + parentOffset;
            
            painter->setPen(QPen(commit.branchColor, lineHeight));
            
            if (i == 0) {
                // Прямая линия вверх для первого родителя
                painter->drawLine(dotX, rect.top(), dotX, dotY - dotRadius);
            } else {
                // Изогнутая линия для остальных родителей (merge)
                QPolygon polygon;
                polygon << QPoint(dotX, dotY - dotRadius)
                        << QPoint(dotX, dotY - 15)
                        << QPoint(parentX, rect.top() + 15)
                        << QPoint(parentX, rect.top());
                painter->drawPolyline(polygon);
            }
        }
    }
}

void CommitItemDelegate::paintCommitItem(QPainter *painter, const QStyleOptionViewItem &option, const CommitData &commit) const
{
    // Устанавливаем цвета
    QColor textColor = option.palette.color(QPalette::Text);
    if (option.state & QStyle::State_Selected) {
        textColor = option.palette.color(QPalette::HighlightedText);
    }

    painter->save();
    painter->setPen(textColor);

    // Получаем rect для рисования
    QRect rect = option.rect;
    rect.setLeft(rect.left() + 110); // Отступ для графа
    rect.setRight(rect.right() - 5);

    // Шрифты
    QFont font = painter->font();
    QFont boldFont = font;
    boldFont.setBold(true);
    
    QFontMetrics fm(font);
    QFontMetrics fmBold(boldFont);

    // Рисуем хэш коммита (жирным)
    painter->setFont(boldFont);
    painter->setPen(commit.branchColor);
    QString hashText = commit.shortHash;
    int hashWidth = fmBold.horizontalAdvance(hashText);
    painter->drawText(rect.left(), rect.top() + fmBold.ascent(), hashText);
    
    // Рисуем refs (ветки, теги)
    if (!commit.refs.isEmpty()) {
        int refsX = rect.left() + hashWidth + 8;
        for (const QString &ref : commit.refs) {
            // Определяем тип ref
            bool isBranch = ref.contains("HEAD") || !ref.contains("tag");
            QColor refColor = isBranch ? QColor(230, 57, 70) : QColor(155, 89, 182);
            
            // Рисуем прямоугольник с текстом
            QString refText = ref;
            int refWidth = fm.horizontalAdvance(refText) + 10;
            QRect refRect(refsX, rect.top() + 2, refWidth, fm.height() + 4);
            
            painter->setPen(refColor);
            painter->setBrush(Qt::transparent);
            painter->drawRoundedRect(refRect, 3, 3);
            
            painter->setPen(refColor);
            painter->setFont(font);
            painter->drawText(refRect.adjusted(5, 0, -5, 0), Qt::AlignVCenter, refText);
            
            refsX += refWidth + 5;
        }
    }
    
    // Рисуем сообщение коммита
    painter->setFont(font);
    painter->setPen(textColor);
    QString messageText = commit.message;
    int messageY = rect.top() + fm.ascent() + 18;
    QString elidedMessage = fm.elidedText(messageText, Qt::ElideRight, rect.width());
    painter->drawText(rect.left(), messageY, elidedMessage);
    
    // Рисуем дату и автора (серым цветом, ниже)
    painter->setPen(Qt::gray);
    QString dateAuthor = commit.author + ", " + commit.date;
    QString elidedDateAuthor = fm.elidedText(dateAuthor, Qt::ElideRight, rect.width());
    painter->drawText(rect.left(), rect.bottom() - fm.descent() - 2, elidedDateAuthor);

    painter->restore();
}
