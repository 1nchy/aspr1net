#ifndef _ASP_ARGUMENTS_HPP_
#define _ASP_ARGUMENTS_HPP_

#include <string>
#include <vector>
#include <cstring>

struct arg_t;

struct arg_t {
    int _type = 0;
    std::vector<std::string> _argv;

    arg_t() = default;
    virtual ~arg_t() {
        _argv.clear();
    }

    void reset() {
        _argv.clear();
        _type = 0;
    }
    int argc() const { return _argv.size(); }
    std::string& argv(size_t _i) { return _argv[_i]; }
    const std::string& argv(size_t _i) const { return _argv[_i]; }
    void process(const char* _s) {
        reset();
        if (_s == nullptr) { return; }
        const char* _p = _s;
        while (1) {
            while (*_p && isspace(*_p)) {
                ++_p;
            }
            const char* _top = _p;
            if (!*_p) {
                return;
            }
            bool _done = false;
            std::string _space_arg;
            while (!_done) {
                switch (*_p) {
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                case '\0':
                    _done = true; break;
                case '\\':
                    if (*(_p+1) == ' ') {
                        _space_arg += std::string(_top, _p);
                        ++_p;
                        _top = _p;
                    }
                default:
                    ++_p;
                }
            }
            if (_space_arg.empty()) {
                _argv.emplace_back(_top, _p);
            }
            else {
                _argv.push_back(_space_arg + std::string(_top, _p));
            }
        }
    }
    void process(const std::string& _s) {
        reset();
        if (_s.empty()) { return; }
        auto _p = _s.cbegin();
        while (1) {
            while (_p != _s.cend() && isspace(*_p)) {
                ++_p;
            }
            if (_p == _s.cend()) { return; }
            auto _top = _p;
            bool _done = false;
            std::string _space_arg;
            while (!_done && _p != _s.cend()) {
                switch (*_p) {
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                case '\0':
                    _done = true; break;
                case '\\':
                    if (_p+1 != _s.cend() && *(_p+1) == ' ') {
                        _space_arg += std::string(_top, _p);
                        ++_p;
                        _top = _p;
                    }
                default:
                    ++_p;
                }
            }
            if (_space_arg.empty()) {
                _argv.emplace_back(_top, _p);
            }
            else {
                _argv.push_back(_space_arg + std::string(_top, _p));
            }
        }
    }
};

#endif // _ASP_ARGUMENTS_HPP_