#include "gitparser.h"
#include <QDebug>

GitParser::GitParser(QObject *parent)
    : QObject(parent)
    , m_hunkHeaderRegex(QStringLiteral("^@@ -(\\d+)(,(\\d+))? \\+(\\d+)(,(\\d+))? @@.*$"))
{
}

QList<Hunk> GitParser::parseDiffOutput(const QString &diffOutput) const
{
    QList<Hunk> hunks;
    
    QStringList lines = diffOutput.split('\n');
    Hunk currentHunk;
    bool inHunk = false;
    
    for (const QString &line : lines) {
        if (line.startsWith("@@")) {
            // This is a hunk header
            QRegularExpressionMatch match = m_hunkHeaderRegex.match(line);
            if (match.hasMatch()) {
                // If we were already processing a hunk, save it
                if (inHunk) {
                    hunks.append(currentHunk);
                }
                
                // Start a new hunk
                int leftStart = match.captured(1).toInt();
                int leftSize = match.captured(3).toInt();
                if (leftSize == 0) leftSize = 1; // Handle special case
                
                int rightStart = match.captured(4).toInt();
                int rightSize = match.captured(6).toInt();
                if (rightSize == 0) rightSize = 1; // Handle special case
                
                currentHunk = Hunk(leftStart, rightStart, leftSize, rightSize);
                currentHunk.lines.clear();
                inHunk = true;
            }
        } else if (inHunk) {
            // Process lines within a hunk
            if (line.startsWith(" ")) {
                // Unchanged line
                currentHunk.lines.append(HunkLine(HunkLine::Unchanged, line.mid(1)));
            } else if (line.startsWith("-")) {
                // Removed line
                currentHunk.lines.append(HunkLine(HunkLine::Removed, line.mid(1)));
            } else if (line.startsWith("+")) {
                // Added line
                currentHunk.lines.append(HunkLine(HunkLine::Added, line.mid(1)));
            } else if (line.startsWith("\\")) {
                // No newline at end of file indicator - skip for now
                continue;
            } else {
                // Could be an empty line or context
                currentHunk.lines.append(HunkLine(HunkLine::Unchanged, line));
            }
        }
        // Ignore other lines (like diff headers, index lines, etc.)
    }
    
    // Don't forget the last hunk
    if (inHunk) {
        hunks.append(currentHunk);
    }
    
    return hunks;
}

QString GitParser::getTypeString(HunkLine::Type type)
{
    switch (type) {
    case HunkLine::Unchanged:
        return QStringLiteral(" ");
    case HunkLine::Removed:
        return QStringLiteral("-");
    case HunkLine::Added:
        return QStringLiteral("+");
    default:
        return QStringLiteral(" ");
    }
}