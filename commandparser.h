#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include <QString>
#include <QStringList>
#include <functional>
#include <map>
#include <vector>

/**
 * Holds a list of commands and their callbacks.
 * Supports:
 * - partial unique prefix matching
 * - 'help' command prints available commands
 * - 'tab' completion (optional: we do partial implementation)
 *
 * Commands:
 *  - up
 *  - down
 *  - left
 *  - right
 *  - goto x y
 *  - attack nearest enemy
 *  - take nearest health pack
 *  - help
 */
class CommandParser {
public:
    struct Command {
        QString fullName;
        std::function<void(QStringList args)> callback;
    };

    CommandParser() {
        // We'll add commands from outside using addCommand()
    }

    void addCommand(const QString &name, std::function<void(QStringList)> cb) {
        commands.push_back({name, cb});
    }

    QStringList getAllCommands() const {
        QStringList list;
        for (auto &c : commands)
            list << c.fullName;
        return list;
    }

    // Find command by partial prefix
    // If unique prefix matches one command, return that command
    // else if multiple matches or none, handle accordingly
    bool parseCommand(const QString &input) {
        QStringList tokens = input.split(' ', Qt::SkipEmptyParts);
        if (tokens.isEmpty()) return false;

        QString firstWord = tokens.first();
        // Check partial match
        std::vector<Command*> matches;
        for (auto &c : commands) {
            if (c.fullName.startsWith(firstWord))
                matches.push_back(&c);
        }

        if (matches.empty()) {
            // no match
            return false;
        } else if (matches.size() > 1) {
            // multiple matches => fail or ask user to be more specific
            return false;
        }

        // unique match
        Command* cmd = matches.front();
        tokens.removeFirst();
        cmd->callback(tokens);
        return true;
    }

    // For help command
    QString helpText() const {
        QString h = "Available commands:\n";
        for (auto &c : commands) {
            h += " - " + c.fullName + "\n";
        }
        return h;
    }

private:
    std::vector<Command> commands;
};

#endif // COMMANDPARSER_H
