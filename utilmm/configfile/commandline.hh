#ifndef UTILMM_COMMANDLINE_HH
#define UTILMM_COMMANDLINE_HH

#include <string>
#include <vector>
#include <list>
// #include <getopt.h>
#include <iosfwd>

namespace utilmm
{
    class config_set;
   
    class bad_syntax : public std::exception
    {
    public:
        ~bad_syntax() throw() {}

        std::string source, error;
        bad_syntax(std::string const& source_, std::string const& error_ = "")
            : source(source_), error(error_) {}
	char const* what() const throw() { return error.c_str(); }
    };

    class commandline_error : public std::exception
    {
    public:
	~commandline_error() throw() {}
	std::string error;
	commandline_error(std::string const& error_)
	    : error(error_) {}
	char const* what() const throw() { return error.c_str(); }
    };

    /** Each option description is parsed and transformed in
     * a cmdline_option object. The command_line code then
     * uses these objects
     */
    class cmdline_option
    {
    public:
        /** Argument types 
         * Each option may have zero or one option
         * the option is described using a or-ed 
         * int of this enum
         */
        enum ArgumentType
        {
            None = 0,         /// no arguments
            Optional = 1,     /// the argument may be ommitted

            IntArgument = 2,    /// the argument is an integer
            BoolArgument = 4,   /// the argument is a boolean, that is 0, 1, 'false' or 'true'
            StringArgument = 8,  /// the argument is a string

	    DefaultValue = 16   /// there is a default value for this argument
        };

    public:
        /** Builds an option using the option description syntax as in @c command_line 
         * @arg option the option description, see the command_line for its syntax 
         * If @c option is not a valid description string, the constructor returns and
         * isValid() will return false */
        cmdline_option(const std::string& description);
        ~cmdline_option();

        bool        isMultiple() const;
	bool	    isRequired() const;
        std::string getConfigKey() const;
        std::string getLong()   const;
        std::string getShort()  const;
        std::string getHelp()   const;

	int getArgumentFlags() const;

        bool hasArgument() const;
        bool isArgumentOptional() const;

	bool hasDefaultValue() const;
	std::string getDefaultValue() const;

        /** Checks that @c value is a valid string according
         * to the argument type (int, bool or string)
         */
        bool checkArgument(const std::string& value) const;

    private:
        bool m_multiple;
	bool m_required;
        std::string m_config, m_long, m_short, m_help;

        int  m_argument_flags;
	std::string m_default;
    };

    /** command_line handling based on getopt_long
     *
     * <h2> Description </h2>
     * 
     * The command_line class allows you to parse user-provided command line
     * options and fill a Config object with them.
     *
     * For each command line option, you must provide a long option (--option_text),
     * may provide a short one (-option_character), and each option may have one 
     * (optional) argument. Eventually, you can give a help string (not used yet).
     *
     * During the parsing, a key/value pair is added to a Config object for each
     * option encountered. The value of the entry is either the option's argument or
     * a boolean value of true if no option is specified.
     *
     * <h2> Usage </h2>
     * 
     * The full syntax is
     * 
     * @code
     *    [!*][config_key]:long_name[,short_name][=value_type[,default]|?value_type,default][:help]
     * @endcode
     *
     * where value_type is one of: int, bool, string
     *
     * When an option is found, an entry is added to a Config object with 
     * the @c config_key key. The value associated is:
     * <ul>
     *    <li> if there is an argument, the value is this argument
     *    <li> if the option takes no argument, it is set to @c true
     *    <li> if the argument is optional and not given, the value
     *    is @c default. @c default is required in case of optional arguments
     * </ul>
     *
     * If @c default is given, the option is set to @c default if it is not
     * found
     *
     * If the option has a mandatory argument, add @c =value_type after the
     * option names. If it is optional, use the @c ?value_type syntax. The '@c
     * int' and '@c bool' value types are checked by the command_line object
     * and an error is generated if the user-provided value dos not match.
     * 
     * <h2> Multiplicity (* and ! options) </h2>
     * When the same option is provided more than once on the command line, the
     * normal behaviour is to use the value of the latest. However, you can
     * also get all the values by adding <tt>*</tt> at the front of the
     * description line. In that case, the config value will a list of values
     * in the config object
     *
     * For instance, an option like the @c -I option of @c gcc will be
     * described using <br> <tt>*:include,I=string|include path</tt>. The
     * result of <tt>gcc -I /a/path -I=/another/path</tt> can then be retrieved
     * with 
     *	<code>
     *	    list<string> includes = config.get< list<string> >('include');
     *	</code>
     *
     * If the ! flag is set, the option is required.
     * 
     * <h2> Examples </h2>
     * 
     * The classical --help option will be given using <tt> :help|display this help and exit </tt>
     *
     * The @c -r and @c --recursive options of grep are described using
     * <tt> :recursive,r|equivalent to --directories=recurse </tt>
     *
     * The @c -m and @c --max-count options of grep are described using
     * <tt> :max-count,m=int|stop after NUM matches</tt>
     */
    class command_line
    {
    private:
        typedef std::vector<cmdline_option> Options;

    public:
        /** Builds an object with a null-terminated string list
         * @param options the option list, null-terminated
         */
        command_line(const char* options[]);

        /** @overload */
        command_line(const std::list<std::string>& description);

        ~command_line();

        /** Parses the command line option
         * @param argc the argument count
         * @param argv the argument value
         * @param config the Config object the option values will be written to
	 *
	 * @throws commandline_error
         */
        void parse(int argc, char const* const argv[], config_set& config);

        /** Remaining command line options
         * After all options are matched, and if no error has occured,
         * this function will return all non-option arguments (input
         * files for instance)
         */
        std::list<std::string> remaining() const;

	/** Sets the first line to appear in usage() */
	void setBanner(std::string const& banner);
    
	/** Outputs a help message to @c out */
	void usage(std::ostream& out) const;

    private:
        void add_argument(config_set& config, cmdline_option const& optdesc, std::string const& value);
	int option_match(config_set& config, cmdline_option const& opt, int argc, char const* const* argv, int i);

	std::string m_banner;
        Options    m_options;
        std::list<std::string> m_remaining;
    };

    std::ostream& operator << (std::ostream& stream, utilmm::command_line const& cmdline)
    { 
	cmdline.usage(stream);
	return stream;
    }
}

#endif

