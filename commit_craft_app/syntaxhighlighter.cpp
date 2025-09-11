#include "syntaxhighlighter.h"

#include <QTextDocument>
#include <QTextCharFormat>
#include <QFont>

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keyword format
    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
                    << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
                    << "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
                    << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
                    << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
                    << "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
                    << "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
                    << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
                    << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
                    << "\\bvoid\\b" << "\\bvolatile\\b" << "\\bbool\\b"
                    << "\\bbreak\\b" << "\\bcase\\b" << "\\bcatch\\b"
                    << "\\bcontinue\\b" << "\\bdefault\\b" << "\\bdelete\\b"
                    << "\\bdo\\b" << "\\belse\\b" << "\\bfalse\\b"
                    << "\\bfor\\b" << "\\bif\\b" << "\\bnew\\b"
                    << "\\bnullptr\\b" << "\\breturn\\b" << "\\bsizeof\\b"
                    << "\\bswitch\\b" << "\\bthis\\b" << "\\bthrow\\b"
                    << "\\btrue\\b" << "\\btry\\b" << "\\bwhile\\b"
                    << "\\binclude\\b" << "\\bdefine\\b" << "\\bifdef\\b"
                    << "\\bifndef\\b" << "\\bendif\\b" << "\\busing\\b"
                    << "\\bnamespace\\b" << "\\bstd\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Class format
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Single line comment format
    singleLineCommentFormat.setForeground(Qt::red);
    rule.pattern = QRegularExpression("//[^]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Multi-line comment format
    multiLineCommentFormat.setForeground(Qt::red);

    // Quotation format (strings)
    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression("\".*?\"");  // Non-greedy string matching
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single quote format (characters)
    rule.pattern = QRegularExpression("'.'");  // Character literals
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Function format
    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Preprocessor directives
    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(Qt::darkYellow);
    preprocessorFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("#[a-zA-Z_]+");
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);
}

void Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);

    // Multi-line comment handling
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf("/*");

    while (startIndex >= 0) {
        QRegularExpression commentEndRegex("\\*/");
        QRegularExpressionMatch match = commentEndRegex.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf("/*", startIndex + commentLength);
    }
}
