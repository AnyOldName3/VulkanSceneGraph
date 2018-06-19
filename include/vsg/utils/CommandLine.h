#pragma once

#include <sstream>

namespace vsg
{

namespace CommandLine
{

    class Convert
    {
    public:
        Convert() {}

        template<typename T>
        bool operator() (const char* field, T& value)
        {
            _str.clear();
            _str.str(field);
            _str >> value;
            return !_str.fail();
        }

        std::istringstream _str;
    };


    void removeArguments(int& argc, char** argv, int pos, int num)
    {
        // remove argument from argv
        for(int i=pos; i<argc-num; ++i)
        {
            argv[i] = argv[i+num];
        }
        argc -= num;
    }

    bool read(int& argc, char** argv, const std::string& match)
    {
        for(int i=1; i<argc; ++i)
        {
            if (match==argv[i])
            {
                removeArguments(argc, argv, i, 1);
                return true;
            }
        }
        return false;
    }

    template<typename T>
    bool read(int& argc, char** argv, const std::string& match, T& value)
    {
        for(int i=1; i<argc; ++i)
        {
            if (match==argv[i])
            {
                if (i+1<argc)
                {
                    Convert convert;

                    T local;
                    if (!convert(argv[i+1],local))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+1]);
                    }

                    value = local;

                    removeArguments(argc, argv, i, 2);

                    return true;
                }
                else
                {
                    throw std::runtime_error(std::string("Warning : Not enough parameters to match : ")+match+" <value>");
                }
            }
        }
        return false;
    }

    template<typename T1, typename T2>
    bool read(int& argc, char** argv, const std::string& match, T1& value1, T2& value2)
    {
        for(int i=1; i<argc; ++i)
        {
            if (match==argv[i])
            {
                if (i+2<argc)
                {
                    Convert convert;

                    T1 local1;
                    if (!convert(argv[i+1],local1))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+1]);
                    }

                    T2 local2;
                    if (!convert(argv[i+2],local2))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+2]);
                    }

                    value1 = local1;
                    value2 = local2;

                    removeArguments(argc, argv, i, 3);

                    return true;
                }
                else
                {
                    throw std::runtime_error(std::string("Warning : Not enough parameters to match : ")+match+" <value>");
                }
            }
        }
        return false;
    }


    template<typename T1, typename T2, typename T3>
    bool read(int& argc, char** argv, const std::string& match, T1& value1, T2& value2, T3& value3)
    {
        for(int i=1; i<argc; ++i)
        {
            if (match==argv[i])
            {
                if (i+3<argc)
                {
                    Convert convert;

                    T1 local1;
                    if (!convert(argv[i+1],local1))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+1]);
                    }

                    T2 local2;
                    if (!convert(argv[i+2],local2))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+2]);
                    }

                    T3 local3;
                    if (!convert(argv[i+3],local3))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+3]);
                    }

                    value1 = local1;
                    value2 = local2;
                    value3 = local3;

                    removeArguments(argc, argv, i, 4);

                    return true;
                }
                else
                {
                    throw std::runtime_error(std::string("Warning : Not enough parameters to match : ")+match+" <value>");
                }
            }
        }
        return false;
    }


    template<typename T1, typename T2, typename T3, typename T4>
    bool read(int& argc, char** argv, const std::string& match, T1& value1, T2& value2, T3& value3, T4& value4)
    {
        for(int i=1; i<argc; ++i)
        {
            if (match==argv[i])
            {
                if (i+4<argc)
                {
                    Convert convert;

                    T1 local1;
                    if (!convert(argv[i+1],local1))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+1]);
                    }

                    T2 local2;
                    if (!convert(argv[i+2],local2))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+2]);
                    }

                    T3 local3;
                    if (!convert(argv[i+3],local3))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+3]);
                    }

                    T4 local4;
                    if (!convert(argv[i+4],local4))
                    {
                        throw std::runtime_error(std::string("Warning : error reading command line parameter : ")+argv[i+4]);
                    }

                    value1 = local1;
                    value2 = local2;
                    value3 = local3;
                    value4 = local4;

                    removeArguments(argc, argv, i, 5);

                    return true;
                }
                else
                {
                    throw std::runtime_error(std::string("Warning : Not enough parameters to match : ")+match+" <value>");
                }
            }
        }
        return false;
    }

} // CommandLine


} // vsg
