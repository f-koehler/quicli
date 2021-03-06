#ifndef QUICLI_HPP_
#define QUICLI_HPP_

#include <algorithm>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace quicli
{
    using Occurance = std::vector<std::string>;

    std::vector<std::string> convert(int argc, char** argv)
    {
        std::vector<std::string> args;
#if defined(_WIN32) || defined(_WIN64)
        auto environ = GetEnvironmentStrings();
        throw std::runtime_error("Not implemented!");
#else
        for(int i = 1; i < argc; ++i) {
            std::string curr(argv[i]);
            auto pos = curr.find('=');
            if(pos == std::string::npos) {
                args.push_back(curr);
                continue;
            }
            args.push_back(curr.substr(0, pos));
            args.push_back(curr.substr(pos + 1));
        }
#endif
        return args;
    }

    std::map<std::string, std::string> environment()
    {
        std::map<std::string, std::string> env;
        std::size_t i = 0;
        while(environ[i] != nullptr) ++i;
        for(auto iter = environ; iter != environ+i; ++iter) {
            std::string entry(*iter);
            auto pos = entry.find_first_of('=');
            env.emplace(std::make_pair(entry.substr(0, pos), entry.substr(pos+1)));
        }
        return env;
    }

    template <typename T>
    using is_vector =
        std::is_same<T, std::vector<typename T::value_type, typename T::allocator_type>>;

    template <typename T>
    using is_list = std::is_same<T, std::list<typename T::value_type, typename T::allocator_type>>;

    template <typename T>
    using is_pair = std::is_same<T, std::pair<typename T::first_name, typename T::second_type>>;

    template <typename T>
    typename std::enable_if<std::is_same<T, double>::value, T>::type as(const std::string& str)
    {
        return std::stod(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, float>::value, T>::type as(const std::string& str)
    {
        return std::stof(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, int>::value, T>::type as(const std::string& str)
    {
        return std::stoi(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, long>::value, T>::type as(const std::string& str)
    {
        return std::stol(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, long double>::value, T>::type as(const std::string& str)
    {
        return std::stold(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, long long>::value, T>::type as(const std::string& str)
    {
        return std::stoll(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, unsigned long>::value, T>::type as(const std::string& str)
    {
        return std::stoul(str);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, unsigned long long>::value, T>::type as(const std::string& str)
    {
        return std::stoull(str);
    }

    template <typename T>
    typename std::enable_if<is_vector<T>::value, T>::type as(const std::string& str)
    {
        T vec;
        std::istringstream strm(str);
        std::string token;
        while(std::getline(strm, token, ','))
            vec.push_back(as<typename T::value_type>(token));
        return vec;
    }

    template <typename T>
    typename std::enable_if<is_list<T>::value, T>::type as(const std::string& str)
    {
        T vec;
        std::istringstream strm(str);
        std::string token;
        while(std::getline(strm, token, ','))
            vec.push_back(as<typename T::value_type>(token));
        return vec;
    }

    template <typename T>
    typename std::enable_if<is_pair<T>::value, T> as(const std::string& str)
    {
        auto pos = str.find(',');
        if(pos == std::string::npos)
            throw std::runtime_error("Did not find comma while parsing pair");
        return std::make_pair(as<typename T::first_type>(str.substr(0, pos)),
                              as<typename T::second_type>(str.substr(pos + 1)));
    }

    class ValueMap : public std::map<std::string, std::vector<Occurance>>
    {
        private:
            std::vector<std::string> _positionals;

        public:
            using map_type = std::map<std::string, std::vector<Occurance>>;
            using map_type::map;

            const std::string& get(const std::string& name) const;
            const Occurance& get_values(const std::string& name) const;
            const std::vector<Occurance>& get_all(const std::string& name) const;
            std::size_t count(const std::string& name) const;

            std::vector<std::string>& positionals();
            const std::vector<std::string>& positionals() const;
    };

    class Argument
    {
        protected:
            std::vector<std::string> _names;
            int _priority;
            std::tuple<bool, std::string> _default;
            bool _mandatory;
            std::string _help;

        public:
            Argument(std::initializer_list<std::string> names);
            Argument(const std::string& name);

            Argument& priority(int val);
            Argument& default_value(const std::string& val);
            Argument& mandatory(bool val);
            Argument& help(const std::string& help);

            const std::string& first_name() const;
            bool has_name(const std::string& name) const;
            int priority() const;
            bool mandatory() const;
            const std::string& help() const;

            std::tuple<bool, std::size_t> matches(const std::string& str) const;
            virtual void extract(const std::tuple<bool, std::size_t>& match,
                                 std::vector<std::string>& args, ValueMap& vm) const = 0;
    };

    class Flag : public Argument
    {
        public:
            using Argument::Argument;

            virtual void extract(const std::tuple<bool, std::size_t>& match,
                                 std::vector<std::string>& args, ValueMap& vm) const override;
    };

    class Parameter : public Argument
    {
        private:
            std::size_t _num_vals;
            bool _multi;

        public:
            Parameter(std::initializer_list<std::string> names);
            Parameter(const std::string& name);

            Parameter& num_values(std::size_t val)
            {
                _num_vals = val;
                return *this;
            }
            Parameter& multi(bool val)
            {
                _multi = val;
                return *this;
            }

            virtual void extract(const std::tuple<bool, std::size_t>& match,
                                 std::vector<std::string>& args, ValueMap& vm) const override;
    };

    class Prompt : public Parameter
    {
        protected:
            std::function<Occurance(void)> _prompt;

        public:
            using Parameter::Parameter;

            Prompt(std::initializer_list<std::string> names, std::function<Occurance(void)> f);

            virtual void extract(const std::tuple<bool, std::size_t>& match,
                                 std::vector<std::string>& args, ValueMap& vm) const override;
    };

    class Interface
    {
        protected:
            std::string _name;
            std::vector<std::unique_ptr<Argument>> _args;
            std::size_t _num_positionals;
            std::string _help_positionals;
            std::string _name_positionals;

        public:
            Interface(const std::string& name);

            template<typename T>
            T& add(const T& arg);

            template<typename T>
            T& get(const std::string& name);

            template<typename T>
            const T& get(const std::string& name) const;

            void parse(int argc, char** argv, ValueMap& vm) const;
            void parse(std::vector<std::string>& args, ValueMap& vm) const;
            void validate(const ValueMap& vm) const;
            virtual std::string help() const;

            Interface& positionals(const std::string& name, const std::string& help = "",
                                   std::size_t num = std::numeric_limits<std::size_t>::max());
    };



    /////////////////////////
    // class ValueMap
    /////////////////////////
    const std::string& ValueMap::get(const std::string& name) const
    {
        auto& occs = at(name);
        if(occs.size() != 1)
            throw std::runtime_error("Argument \"" + name + "\" was passed more than once");
        auto& occ = occs.front();
        if(occ.size() != 1)
            throw std::runtime_error("Argument \"" + name + "\" was passed with more than one value");
        return occ.front();
    }
    const Occurance& ValueMap::get_values(const std::string& name) const
    {
        auto& occs = at(name);
        if(occs.size() != 1)
            throw std::runtime_error("Argument \"" + name + "\" was passed more than once");
        return occs.front();
    }
    const std::vector<Occurance>& ValueMap::get_all(const std::string& name) const
    {
        return at(name);
    }
    std::size_t ValueMap::count(const std::string& name) const
    {
        auto pos = find(name);
        if(pos == end()) return 0;
        return pos->second.size();
    }

    std::vector<std::string>& ValueMap::positionals() { return _positionals; }
    const std::vector<std::string>& ValueMap::positionals() const { return _positionals; }


    /////////////////////////
    // class Argument
    /////////////////////////
    Argument::Argument(std::initializer_list<std::string> names) : _names(names), _mandatory(false)
    {
    }
    Argument::Argument(const std::string& name) : _names{name}, _mandatory(false) {}

    Argument& Argument::priority(int val)
    {
        _priority = val;
        return *this;
    }
    Argument& Argument::default_value(const std::string& val)
    {
        std::get<1>(_default) = val;
        return *this;
    }
    Argument& Argument::mandatory(bool val)
    {
        _mandatory = val;
        return *this;
    }

    Argument& Argument::help(const std::string& help)
    {
        _help = help;
        return *this;
    }

    const std::string& Argument::first_name() const { return _names.front(); }
    bool Argument::has_name(const std::string& name) const
    {
        return std::find(_names.begin(), _names.end(), name) != _names.end();
    }
    int Argument::priority() const { return _priority; }
    bool Argument::mandatory() const { return _mandatory; }
    const std::string& Argument::help() const { return _help; }

    std::tuple<bool, std::size_t> Argument::matches(const std::string& str) const
    {
        auto iter = std::find(_names.begin(), _names.end(), str);
        if(iter == _names.end()) return std::make_tuple(false, 0);
        return std::make_tuple(true, iter - _names.begin());
    }



    /////////////////////////
    // class Flag
    /////////////////////////
    void Flag::extract(const std::tuple<bool, std::size_t>& match, std::vector<std::string>& args,
                         ValueMap& vm) const
    {
        static auto count = 0ul;
        if(!std::get<0>(match)) return;
        args.erase(args.begin());
        ++count;
        auto pos = vm.find(_names.front());
        if(pos == vm.end())
            vm.insert(std::make_pair(_names.front(), std::vector<Occurance>{{"init"}}));
        vm[_names.front()][0][0] = std::to_string(count);
    }



    /////////////////////////
    // class Parameter
    /////////////////////////
    Parameter::Parameter(std::initializer_list<std::string> names)
        : Argument(names), _num_vals(1), _multi(false)
    {
    }

    Parameter::Parameter(const std::string& name)
        : Argument(name), _num_vals(1), _multi(false)
    {
    }

    void Parameter::extract(const std::tuple<bool, std::size_t>& match, std::vector<std::string>& args,
                 ValueMap& vm) const
    {
        if(!std::get<0>(match)) return;
        args.erase(args.begin());
        if(_num_vals > args.size())
            throw std::runtime_error("Argument \"" + first_name() + "\" requires "
                                     + std::to_string(_num_vals) + " values, but only got "
                                     + std::to_string(args.size()));
        auto pos = vm.find(_names.front());
        if(pos == vm.end()) vm.insert(std::make_pair(_names.front(), std::vector<Occurance>()));
        vm[_names.front()].emplace_back(Occurance(args.begin(), args.begin() + _num_vals));
        args.erase(args.begin(), args.begin() + _num_vals);
    }



    /////////////////////////
    // class Prompt
    /////////////////////////
    Prompt::Prompt(std::initializer_list<std::string> names, std::function<Occurance(void)> f)
        : Parameter(names), _prompt(f)
    {
    }

    void Prompt::extract(const std::tuple<bool, std::size_t>& match, std::vector<std::string>& args,
                         ValueMap& vm) const
    {
        if(!std::get<0>(match)) return;
        args.erase(args.begin());
        auto occ = _prompt();
        auto pos = vm.find(_names.front());
        if(pos == vm.end()) vm.insert(std::make_pair(_names.front(), std::vector<Occurance>()));
        vm[_names.front()].push_back(occ);
    }



    /////////////////////////
    // class Interface
    /////////////////////////
    Interface::Interface(const std::string& name) : _name(name) {}

    template <typename T>
    T& Interface::add(const T& arg)
    {
        auto pos = std::find_if(_args.begin(), _args.end(), [&arg](std::unique_ptr<Argument>& a) {
            return a->priority() <= arg.priority();
        });
        T* tmp = new T(arg);
        _args.insert(pos, std::unique_ptr<T>(tmp));
        return *tmp;
    }

    template <typename T>
    T& Interface::get(const std::string& name)
    {
        auto pos =
            std::find_if(_args.begin(), _args.end(),
                         [&name](std::unique_ptr<Argument>& arg) { return arg->has_name(name); });
        if(pos == _args.end())
            throw std::runtime_error("No argument with name \"" + name + "\" in Interface \"" + _name
                                     + "\"");
        return *dynamic_cast<T*>(pos->get());
    }

    template<typename T>
    const T& Interface::get(const std::string& name) const
    {
        auto pos =
            std::find_if(_args.begin(), _args.end(), [&name](const std::unique_ptr<Argument>& arg) {
                return arg->has_name(name);
            });
        if(pos == _args.end())
            throw std::runtime_error("No argument with name \"" + name + "\" in Interface \"" + _name
                                     + "\"");
        return *dynamic_cast<const T*>(pos->get());
    }

    void Interface::parse(int argc, char** argv, ValueMap& vm) const
    {
        auto args = convert(argc, argv);
        parse(args, vm);
    }

    void Interface::parse(std::vector<std::string>& args, ValueMap& vm) const
    {
        while(!args.empty()) {
            bool matched = false;
            for(auto& arg : _args) {
                auto match = arg->matches(args.front());
                if(!std::get<0>(match)) continue;
                arg->extract(match, args, vm);
                matched = true;
                break;
            }

            if(_num_positionals) {
                auto count = std::min(args.size(), _num_positionals);
                std::copy(args.begin(), args.begin() + count, std::back_inserter(vm.positionals()));
                args.erase(args.begin(), args.begin() + count);
                matched = args.empty();
            }

            if(!matched) throw std::runtime_error("Argument \"" + args.front() + "\" is unmatched");
        }
    }

    void Interface::validate(const ValueMap& vm) const
    {
        std::string msg = "";
        for(auto& arg : _args) {
            if(!arg->mandatory()) continue;
            auto pos = vm.find(arg->first_name());
            if(pos == vm.end()) msg += "Mandatory argument \"" + arg->first_name() + "\" not found\n";
        }
        if(!msg.empty()) throw std::runtime_error("Invalid set of arguments! Errors:\n"+msg);
    }

    std::string Interface::help() const
    {
        std::ostringstream strm;
        strm << _name;
        if(_num_positionals) strm << " " << _name_positionals;
        strm << std::endl << std::endl;

        if(_num_positionals) {
            strm << _name_positionals << "\t" << _help_positionals << std::endl;
        }

        for(auto& entry : _args) {
            strm << entry->first_name() << "\t" << entry->help() << std::endl;
        }
        return strm.str();
    }

    Interface& Interface::positionals(const std::string& name, const std::string& help,
                                      std::size_t num)
    {
        _name_positionals = name;
        _help_positionals = help;
        _num_positionals  = num;
        return *this;
    }
}

#endif
