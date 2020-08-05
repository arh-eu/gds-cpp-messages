#ifndef ARG_PARSE_HPP
#define ARG_PARSE_HPP

#if __has_include(<getopt.h>)
    #include <getopt.h>
#else
    #error "Using this program requires the '<getopt.h>' header from the *NIX standard!"
#endif
#include <array>
#include <map>
#include <string>

class ArgParser {
    private:
        const int argc;
        char* const* const argv;
        std::map<std::string, std::string> m_values;
        std::string msg;

    public:
        explicit ArgParser(int _argc, char** const _argv) : argc(_argc), argv(_argv)
        {
            m_values["username"] = "user";
            m_values["timeout"] = "30";
            m_values["url"] = "127.0.0.1:8888/gate";
        }

        const std::string& message() const 
        {
            return msg;
        }

        bool has_arg(const std::string& key) const noexcept
        {
            return m_values.find(key) != m_values.end();
        }

        std::string get_arg(const std::string& key) const 
        {
            return m_values.at(key);
        }

        bool parse()
        {
            static struct option long_options[] = 
            {
                {"username", required_argument, NULL, 'n'},
                {"password", required_argument, NULL, 'p'},
                {"url", required_argument, NULL, 'c'}, //
                {"timeout", required_argument, NULL, 't'},
                {"hex", required_argument, NULL, 'x'},
                {"help", no_argument, NULL, 's'},
                //{"tls", no_argument, NULL, 's'},


                {"attachment", required_argument, NULL, 'a'},
                {"attachments", required_argument, NULL, 'f'},
                {"event", required_argument, NULL, 'e'},
                {"insert", required_argument, NULL, 'i'},
                {"merge", required_argument, NULL, 'm'},
                {"query", required_argument, NULL, 'q'},
                {"queryall", required_argument, NULL, 'r'},
                {"update", required_argument, NULL, 'u'},

            };
            
            signed char ch;
            while((ch = getopt_long_only(argc, argv, "", long_options, NULL)) != -1)
            {
                switch(ch)
                {
                    case 'n':
                        m_values["username"] = std::string(optarg);
                        break;

                    case 'p':
                        m_values["password"] = std::string(optarg);
                        break;

                    case 'c':
                        m_values["url"] = std::string(optarg);
                        break;
                    
                    case 't':
                        m_values["timeout"] = std::string(optarg);
                        break;

                    case 'x':
                        m_values["hex"] = std::string(optarg);
                        break;

                    case 'e':
                    case 'i':
                    case 'm':
                    case 'u':
                        if (check_has_commands()){
                            return false;
                        }
                        m_values["event"] = std::string(optarg);
                        break;
                        
                    case 'f':
                        m_values["files"] = std::string(optarg);
                        break;

                    case 'q':
                        if (check_has_commands()){
                            return false;
                        }
                        m_values["query"] = std::string(optarg);
                        break;

                    case 'r':
                        if (check_has_commands()){
                            return false;
                        }
                        m_values["queryall"] = std::string(optarg);
                        break;

                    case 'a':
                        if (check_has_commands()){
                            return false;
                        }
                        m_values["attachment"] = std::string(optarg);
                        break;
                    case 's':
                        create_help();
                        return false;

                    case '?':
                    case -1:
                        return false;
                }
            }


            if(check_has_commands())
            {
                return true;
            }
            else
            {
                msg = "No argument was specified, cannot run without any!";
                return false;
            }
        }

        private:
        bool check_has_commands() const noexcept
        {
            return (has_arg("attachment") || has_arg("event") || has_arg("query") || has_arg("queryall"));
        }

        void create_help() noexcept
        {
            msg = "Usage: " + std::string(argv[0]);
            msg += " [-help] [-username USERNAME] [-password PASSWORD] [-timeout TIMEOUT] [-url URL]\n";
            msg += "(-hex HEX | -attachment ATTACHMENT | -insert INSERT | -event EVENT | -merge MERGE | -query QUERY | -queryall QUERY) | -update UPDATE)";
            msg += "[-attachments ATTACHMENTS]";
        }
};

#endif // ARG_PARSE_HPP