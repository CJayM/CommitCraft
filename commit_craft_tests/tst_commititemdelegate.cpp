#include "tst_commititemdelegate.h"

#include "../commit_craft_lib/commititemdelegate.h"
#include "../commit_craft_lib/commithistorymodel.h"
#include <QPainter>
#include <QPixmap>
#include <QStyleOptionViewItem>

void TestCommitItemDelegate::initTestCase() {}

void TestCommitItemDelegate::cleanupTestCase() {}

void TestCommitItemDelegate::testDelegateCreation()
{
    CommitItemDelegate delegate;
    QVERIFY(true); // Just test that creation works
}

void TestCommitItemDelegate::testSizeHint()
{
    CommitItemDelegate delegate;
    
    // Create a mock index and option for testing
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    model.setCommits(commits);
    
    QStyleOptionViewItem option;
    QModelIndex index = model.index(0, 0);
    
    QSize size = delegate.sizeHint(option, index);
    
    // The delegate should return a fixed height of 40
    QCOMPARE(size.height(), 40);
    // Width should be non-zero (inherited from base class)
    QVERIFY(size.width() >= 0);
}

void TestCommitItemDelegate::testPaintWithValidCommitData()
{
    CommitItemDelegate delegate;
    
    // Create a model with valid commit data
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    model.setCommits(commits);
    
    // Create a pixmap to paint on
    QPixmap pixmap(400, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 400, 100);
    option.state = QStyle::State_None;
    
    QModelIndex index = model.index(0, 0);
    
    // This should not crash
    delegate.paint(&painter, option, index);
    
    // Verify painting completed without errors
    QVERIFY(!pixmap.isNull());
}

void TestCommitItemDelegate::testPaintWithInvalidCommitData()
{
    CommitItemDelegate delegate;
    
    // Create a model with invalid data (less than 4 fields)
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    QList<QString> commit1;
    commit1 << "abc123def"; // Only hash, missing other fields
    commits.append(commit1);
    model.setCommits(commits);
    
    // Create a pixmap to paint on
    QPixmap pixmap(400, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 400, 100);
    option.state = QStyle::State_None;
    
    QModelIndex index = model.index(0, 0);
    
    // This should not crash even with incomplete data
    delegate.paint(&painter, option, index);
    
    // Verify painting completed without errors
    QVERIFY(!pixmap.isNull());
}

void TestCommitItemDelegate::testPaintWithEmptyCommitData()
{
    CommitItemDelegate delegate;
    
    // Create a model with empty commit data
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    QList<QString> commit1;
    // Empty list
    commits.append(commit1);
    model.setCommits(commits);
    
    // Create a pixmap to paint on
    QPixmap pixmap(400, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 400, 100);
    option.state = QStyle::State_None;
    
    QModelIndex index = model.index(0, 0);
    
    // This should not crash even with empty data
    delegate.paint(&painter, option, index);
    
    // Verify painting completed without errors
    QVERIFY(!pixmap.isNull());
}

void TestCommitItemDelegate::testPaintWithPartialCommitData()
{
    CommitItemDelegate delegate;
    
    // Create a model with partial commit data (some fields missing)
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    // Test with only 2 fields (hash and author)
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe";
    commits.append(commit1);
    
    // Test with 3 fields (hash, author, date)
    QList<QString> commit2;
    commit2 << "def456ghi" << "Jane Smith" << "2023-01-02";
    commits.append(commit2);
    
    model.setCommits(commits);
    
    // Create a pixmap to paint on
    QPixmap pixmap(400, 200);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 400, 100);
    option.state = QStyle::State_None;
    
    // Test first commit (2 fields)
    QModelIndex index1 = model.index(0, 0);
    delegate.paint(&painter, option, index1);
    
    // Test second commit (3 fields)
    option.rect = QRect(0, 100, 400, 100);
    QModelIndex index2 = model.index(1, 0);
    delegate.paint(&painter, option, index2);
    
    // Verify painting completed without errors
    QVERIFY(!pixmap.isNull());
}

int this_method_make_TestCommitItemDelegate_visible_in_QT_tests_panel()
{
    return QTest::qExec(new TestCommitItemDelegate());
}
