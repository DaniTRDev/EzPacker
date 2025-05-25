class AstBuilder:
    """
    This class will be of use when defining new AstNodes. It's responsible for creating a new node, define its parsing rules
    and its variables.
    """

    def __init__(self):
        self.member_name: str = ""  # Variable that is defined in this node.
        self.member_type: str = ""  # Variable's type.
        self.name_string: str = ""
        self.token_content: str = ""  # Name of the token type that will fill node's member.
        self.error_string: str = ""  # String that will be shown if parser fails processing this node.

    def error(self, error_str: str):
        self.error_string = error_str
        return self

    def member_from_token(self, member_name: str, token_content: str):
        """
        Adds a member of the node that will be filled when a certain token is passed into its parsing rule.
        :param member_name:
        :param token_content:
        :return:
        """

        self.member_name = member_name
        self.member_type = "std::string"
        self.token_content = token_content
        return self

    def member_from_constructor(self, member_name: str, member_type: str):
        """
        Adds a member to the node that will be filled with a value given in the constructor.
        :param member_name:
        :param member_type:
        :return:
        """
        self.member_name = member_name
        self.member_type = member_type
        return self

    def name(self, name: str):
        """
        Sets the name of the Ast node. Replaces existing.
        :param name:
        :return:
        """
        self.name_string = str(name)
        return self


# These are all supported Ast nodes of our parser. If a new node is added, just declare it here and push to the node
# list.

# Used as intermediate when generating nodes that are going to be pushed into parent nodes.
null_node = AstBuilder().name("Null")

identifier_node = AstBuilder().name("Identifier").member_from_token("Content", "Identifier")
instruction_node = AstBuilder().name("Instruction")
instruction_operand_node = AstBuilder().name("InstructionOperand")
keyword_node = AstBuilder().name("Keyword")
memory_node = AstBuilder().name("Memory").member_from_constructor("refType", "MemoryReferenceType")
module_header_node = AstBuilder().name("Module header")
module_node = AstBuilder().name("Module")
int_number_node = AstBuilder().name("IntNumber").member_from_token("NumberContent", "NumberInt")
float_number_node = AstBuilder().name("FloatNumber").member_from_token("NumberContent", "NumberFloat")
string_node = AstBuilder().name("String").member_from_token("StringContent", "String")
type_node = AstBuilder().name("Type")
variable_node = AstBuilder().name("Variable")
virtual_variable_node = AstBuilder().name("VirtualVariable")

ast_node_list: list[AstBuilder] = [identifier_node, instruction_node, keyword_node, module_node, int_number_node,
                                   float_number_node, string_node, type_node, variable_node, virtual_variable_node]
