#ifndef GITPARSER_H
#define GITPARSER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QRegularExpression>

struct HunkLine {
    enum Type {
        Unchanged = 0,
        Removed = 1,
        Added = 2
    };
    
    Type type;
    QString content;
    
    HunkLine(Type t = Unchanged, const QString &c = QString()) : type(t), content(c) {}
};

struct Hunk {
    int leftStart;      // Starting line number in the original file
    int rightStart;     // Starting line number in the modified file
    int leftSize;       // Number of lines in the original file
    int rightSize;      // Number of lines in the modified file
    QString caption;    // Caption from the hunk header (text after second @@)
    QList<HunkLine> lines;  // Lines in this hunk
    
    Hunk() : leftStart(0), rightStart(0), leftSize(0), rightSize(0) {}
    
    Hunk(int ls, int rs, int lsz, int rsz, const QString &cap = QString()) 
        : leftStart(ls), rightStart(rs), leftSize(lsz), rightSize(rsz), caption(cap) {}
};

class GitParser : public QObject
{
    Q_OBJECT

public:
    explicit GitParser(QObject *parent = nullptr);
    
    // Parse git diff output and extract hunks
    QList<Hunk> parseDiffOutput(const QString &diffOutput) const;
    
    // Helper methods
    static QString getTypeString(HunkLine::Type type);

private:
    // Regular expressions for parsing
    QRegularExpression m_hunkHeaderRegex;
};

#endif // GITPARSER_H