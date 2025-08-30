import gdb


class IdStringPrinter:
    """
    Pretty printer for hdl::IdString class.

    This printer displays the actual string content of IdString objects by
    calling the c_str() method on the std::string returned by resolveGlobal().
    This approach converts the std::string to a C-style string pointer that
    can be easily handled by GDB's string() method.
    """
    def __init__(self, val):
        """
        Initialize the pretty printer with a GDB value.

        Args:
            val (gdb.Value): The GDB value representing an IdString object
        """
        self.val = val
        self.mId = int(val['mId'])

    def to_string(self):
        """
        Convert the IdString to its string representation.

        This method calls resolveGlobal() to get the std::string, then calls
        c_str() on it to convert it to a C-style string that can be processed
        by GDB's string() method.

        Returns:
            str: The string content of the IdString, or an error message if resolution fails
        """
        # Handle invalid IdString values (0xFFFFFFFF represents an invalid ID)
        if self.mId == 0xFFFFFFFF:
            return "<Invalid>"

        try:
            # Call resolveGlobal() to get the std::string, then immediately call c_str() on it
            # This converts the std::string to a C-style string (char*) that GDB can handle
            result = gdb.parse_and_eval(
                f"hdl::IdString::resolveGlobal({self.mId}).c_str()")

            # Use GDB's string() method to convert the char* to a Python string
            return result.string()

        except gdb.error as e:
            # If the above approach fails, try a fallback method
            return self.fallback_resolve()
        except Exception as e:
            return f"<IdString: {self.mId} (Unexpected error: {e})>"

    def fallback_resolve(self):
        """
        Fallback method for resolving IdString content when the primary method fails.

        This method uses gdb.execute() to call resolveGlobal() and parses the output.
        It's less efficient than the primary method but can work in cases where
        parse_and_eval() has issues with the expression.

        Returns:
            str: The string content, or an error message if resolution fails
        """
        try:
            # Use gdb.execute() to call resolveGlobal() and capture the output
            output = gdb.execute(
                f"print hdl::IdString::resolveGlobal({self.mId})",
                to_string=True,
                from_tty=False)

            # Parse the output to extract the string content
            # GDB typically formats output as: $num = "string content"
            lines = output.split('\n')
            for line in lines:
                if '"' in line and '=' in line:
                    # Look for the assignment pattern in the output
                    parts = line.split('=')
                    if len(parts) > 1:
                        value = parts[1].strip()
                        # Extract the quoted string content
                        if value.startswith('"') and value.endswith('"'):
                            return value[1:-1]

            # If parsing fails, return a portion of the raw output for debugging
            return f"<Cannot parse GDB output: {output[:100]}...>"
        except Exception as e:
            return f"<Fallback method also failed: {e}>"

    def display_hint(self):
        """
        Provide a hint to GDB about how to display this value.

        Returns:
            str: The display hint ('string' in this case)
        """
        return 'string'


def register_idstring_printers():
    """
    Register the IdString pretty printers with GDB.

    This function creates a regex-based pretty printer collection and
    registers it with GDB's current object file. The printer will handle
    various forms of IdString references and pointers.
    """
    # Create a regex-based pretty printer collection
    pp = gdb.printing.RegexpCollectionPrettyPrinter("IdString")

    # Register patterns for different forms of IdString references
    pp.add_printer('IdString', '^hdl::IdString$', IdStringPrinter)
    pp.add_printer('IdString', '^const hdl::IdString&$', IdStringPrinter)
    pp.add_printer('IdString', '^hdl::IdString&$', IdStringPrinter)
    pp.add_printer('IdString', '^hdl::IdString\*$', IdStringPrinter)
    pp.add_printer('IdString', '^const hdl::IdString\*$', IdStringPrinter)

    # Register the pretty printer with GDB
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp, True)


# Register the pretty printers when this script is loaded
register_idstring_printers()
