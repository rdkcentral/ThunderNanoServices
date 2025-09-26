#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

class TerminalInput {
public:
    typedef void (*InputHandler)(char);

    TerminalInput()
        : _original()
        , _originalFlags(0)
        , _valid(SetupTerminal())
    {
    }

    ~TerminalInput()
    {
        if (_valid) {
            RestoreTerminal();
        }
    }

    TerminalInput(const TerminalInput&) = delete;
    TerminalInput& operator=(const TerminalInput&) = delete;
    TerminalInput(TerminalInput&&) = delete;
    TerminalInput& operator=(TerminalInput&&) = delete;

    bool IsValid() const { return _valid; }

    char Read()
    {
        char c = 0;
        read(STDIN_FILENO, &c, 1);
        return c;
    }

private:
    bool SetupTerminal()
    {
        if (tcgetattr(STDIN_FILENO, &_original) != 0) {
            return false;
        }

        _originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (_originalFlags == -1) {
            return false;
        }

        struct termios raw = _original;

        raw.c_lflag &= ~(ICANON | ECHO); // No line buffering, no echo
        raw.c_cc[VMIN] = 0; // Non-blocking read
        raw.c_cc[VTIME] = 0; // No timeout

        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
            return false;
        }

        if (fcntl(STDIN_FILENO, F_SETFL, _originalFlags | O_NONBLOCK) == -1) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &_original);
            return false;
        }

        return true;
    }

    void RestoreTerminal()
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &_original);
        fcntl(STDIN_FILENO, F_SETFL, _originalFlags);
    }

private:
    struct termios _original;
    int _originalFlags;
    bool _valid;
};