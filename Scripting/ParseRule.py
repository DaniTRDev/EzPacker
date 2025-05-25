import copy

import AstBuilder


class ParseRule:
    """
    This class represents a ParseRule that's defined in EzParser.
    """

    def __init__(self):
        self.is_inherited_from_project: bool = False
        self.name_string: str = ""
        self.match_func_content: str = ""  # Function used to know if this rule was applied or not.
        self.callback_func_content: str = ""  # Function called when this rule was successfully applied.
        self.match_rules_content: list[ParseRule] = []
        self.argument_name_array: list[str] = []
        self.argument_type_array: list[str] = []
        self.call_with_array: list[str] = []
        self.map_to: str = ""  # To which type this rule should be mapped into? (Resulting nodes are going
        # to be pushed into current node or into parent node?)
        self.map_arguments: list[str] = []

    def match_rules(self, match_rules_content: list[object]):
        """
        This method is used to define a set of rules that will be used to match this rule. It will REPLACE
        any given match_func behaviour.
        :param match_rules_content:
        :return:
        """
        self.match_rules_content = match_rules_content
        return self

    def match_rule(self, match_rule_content: object):
        """
        This method is used to define a set of rules that will be used to match this rule. It will REPLACE
        any given match_func behaviour.
        :param match_rule_content:
        :return:
        """
        self.match_rules_content += match_rule_content
        return self

    def match_func(self, match_func_content: str):
        """
        Sets the matcher (replaces existing) for this rule.
        :param match_func_content:
        :return:
        """
        self.match_func_content = match_func_content
        return self

    def name(self, name_string: str):
        """
        This function sets the name of the rule. Will be used for error reporting.
        :param name_string:
        :return:
        """
        self.name_string = name_string
        return self

    def callback(self, callback_func_content: str):
        """
        Sets the callback that will be executed if this rule has been successfully applied.
        :param callback_func_content:
        :return:
        """
        self.callback_func_content = callback_func_content
        return self

    def inherit_from_project(self):
        self.is_inherited_from_project = True
        return self

    def new(self):
        return copy.deepcopy(self)

    def argument(self, argument_name: str, argument_type: str):
        """
        Adds an argument of the given type into the argument list for this rule.
        :param argument_name:
        :param argument_type:
        :return:
        """
        self.argument_name_array += argument_name
        self.argument_type_array += argument_type
        return self

    def call_with(self, argument_list: list[str]):
        """
        Sets the rule to be called with the given argument list.
        :param argument_list:
        :return:
        """
        self.call_with_array = argument_list
        return self

    def map(self, map_type: str):
        """
        Sets the type to which generated nodes of this rule will be mapped into. Will replace CALLBACK.
        :param map_type:
        :return:
        """
        self.map_to = map_type
        return self

    def map_with_args(self, map_type: str, argument_list: list[str]):
        """
        Same as map but it includes arguments in the constructor
        :param map_type:
        :param argument_list:
        :return:
        """
        self.map(map_type)
        self.map_arguments = argument_list
        return self

    def _header_to_code(self):
        """
        Converts current layout into a valid C++ function header.
        :return:
        """
        arguments_content = ""
        current_arg_id = 0
        for arg_name, arg_type in self.argument_name_array, self.argument_type_array:
            arguments_content += f"{arg_type} {arg_name}"

            if current_arg_id != len(self.argument_name_array):
                arguments_content += ", "

            current_arg_id += 1

        header_content = f"std::shared_ptr<ParseRule> {self.name_string}({arguments_content})"
        return header_content

    def to_code(self):
        """
        Converts the current layout to C++ code.
        :return:
        """
        header: str = self._header_to_code()

        body = ""
        for rule in self.match_rules_content:
            rule_call_content = f"{rule.name_string}({rule.call_with_array})"



# Now we are going to define parsing rules for each of the nodes.

# This section defines rules that WILL be already defined in the project (Operators).
optional = ParseRule().name("Optional").inherit_from_project()
sequence = ParseRule().name("Sequence").inherit_from_project()
any_of = ParseRule().name("AnyOf").inherit_from_project()
match_if = ParseRule().name("MatchIf").inherit_from_project()
many_of = ParseRule().name("ManyOf").inherit_from_project()

# Token type rule will only be used to "check" if a token is present or not, can't be used to get the content of a token
# by itself.
token_type_rule = ParseRule().name("TokenType").argument("type", "IRTokenType").match_func("""
    auto &token = parser.peek();
    if (!parser.consumeIfToken(type))
    {
        parser.getErrorCollector()->enterRule();
        parser.getErrorCollector()->collect(LogMessage("").add("Expected {} but got {}", g_IRTokenTypeStr[type],
                                                                       g_IRTokenTypeStr[token.m_type]),
                                            parser.peek().m_sourceReference);
        parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
        return false;
    }
    
    return true;
""")

identifier_rule = ParseRule().name("Identifier").match_func(f"""
    auto &token = parser.peek();
    if (!parser.consumeIfToken({AstBuilder.identifier_node.token_content}))
    {{
        parser.getErrorCollector()->enterRule();
        parser.getErrorCollector()->collect(LogMessage("").add("Expected {{}} but got {{}}", g_IRTokenTypeStr[type],
                                                                       g_IRTokenTypeStr[token.m_type]),
                                            parser.peek().m_sourceReference);
        parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
        return false;
    }}
    
    auto identifier = std::make_shared<{AstBuilder.identifier_node.member_name}>();
    identifier->set{AstBuilder.identifier_node.member_name}(token.m_str);
    identifier->setSourceRef(token.m_sourceReference);
    
    out->addChild(std::move(identifier));
    
    return true;
""")

number_rule = ParseRule().name("Number").match_func(f"""
    auto &token = parser.peek();
    if (!parser.consumeIfToken({AstBuilder.int_number_node.token_content}) && 
        !parser.consumeIfToken({AstBuilder.float_number_node.token_content}))
    {{
        parser.getErrorCollector()->enterRule();
        parser.getErrorCollector()->collect(LogMessage("").add("Expected {{}} but got {{}}", g_IRTokenTypeStr[type],
                                                                       g_IRTokenTypeStr[token.m_type]),
                                            parser.peek().m_sourceReference);
        parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
        return false;
    }}
    
    switch(token.m_type)
    {{
        case IRTokenType::{AstBuilder.int_number_node.token_content}: {{ 
            auto num = std::make_shared<{AstBuilder.int_number_node.member_name}>();
            num->set{AstBuilder.int_number_node.member_name}(token.m_str);
            num->setSourceRef(token.m_sourceReference);
    
            out->addChild(std::move(num));    
        
        break; 
        }}
        case IRTokenType::{AstBuilder.float_number_node.token_content}: {{ 
            auto num = std::make_shared<{AstBuilder.float_number_node.member_name}>();
            num->set{AstBuilder.float_number_node.member_name}(token.m_str);
            num->setSourceRef(token.m_sourceReference);
    
            out->addChild(std::move(num));    
        
        break; 
        }}
        default: {{ 
            parser.getErrorCollector()->enterRule();
            parser.getErrorCollector()->collect(LogMessage("").add("Invalid number token"), token.m_sourceReference);
            parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
            return false;
        }}
    }}
        
    return true;
""")

string_rule = ParseRule().name("String").match_func(f"""
    auto &token = parser.peek();
    if (!parser.consumeIfToken({AstBuilder.string_node.token_content}))
    {{
        parser.getErrorCollector()->enterRule();
        parser.getErrorCollector()->collect(LogMessage("").add("Expected {{}} but got {{}}", g_IRTokenTypeStr[type],
                                                                       g_IRTokenTypeStr[token.m_type]),
                                            parser.peek().m_sourceReference);
        parser.getErrorCollector()->exitRule(ErrorHandleType::Propagate);
        return false;
    }}
    
    auto identifier = std::make_shared<{AstBuilder.string_node.member_name}>();
    identifier->set{AstBuilder.string_node.member_name}(token.m_str);
    identifier->setSourceRef(token.m_sourceReference);
    
    out->addChild(std::move(identifier));
    
    return true;
""")

keyword_rule = ParseRule().name("Keyword").match_rule(sequence.new().match_rules([
    token_type_rule.new().call_with(["IRTokenType::Dot"]),
    identifier_rule.new()
])).map(AstBuilder.keyword_node.name_string)

type_rule = ParseRule().name("Type").match_rule(sequence.new().match_rules([
    token_type_rule.new().call_with(["IRTokenType::Dot"]),
    identifier_rule.new()
])).map(AstBuilder.type_node.name_string)

virtual_variable_rule = ParseRule().name("VirtualVariable").match_rule(sequence.new().match_rules([
    type_rule.new(),
    token_type_rule.new().call_with(["IRTokenType::Percentage"]),
    identifier_rule.new()
])).map(AstBuilder.virtual_variable_node.name_string)

variable_rule = ParseRule().name("Variable").match_rule(sequence.new().match_rules([
    keyword_rule.new(),
    identifier_rule.new(),
    token_type_rule.new().call_with(["IRTokenType::Colon"]),
    type_rule.new(),
    any_of.new().match_rules([number_rule.new(), string_rule.new()]),
    optional.new().match_rules([
        many_of.new().call_with(["0", "UINT64_MAX"]).match_rules([
            token_type_rule.new().call_with(["IRTokenType::Comma"]),
            any_of.new().match_rules([number_rule.new(), string_rule.new()])
        ])
    ])
])).map(AstBuilder.variable_node.name_string)

# Memory nodes has a lot of variants, one for each addressing mode.
base_memory_rule = ParseRule().name("Base").match_rule(sequence.new().match_rule(
    virtual_variable_rule.new())).map_with_args(AstBuilder.memory_node.name_string, ["MemoryReferenceType::Base"])

base_displ_memory_rule = ParseRule().name("BaseDispl").match_rule(sequence.new().match_rules([
    virtual_variable_rule.new(),
    token_type_rule.new().call_with(["IRTokenType::Comma"]),
    number_rule.new()
])).map_with_args(AstBuilder.memory_node.name_string, ["MemoryReferenceType::BaseDispl"])

base_index_scale_displ_memory_rule = ParseRule().name("BaseIndexScaleDispl").match_rule(sequence.new().match_rules([
    virtual_variable_rule.new(),  # base
    token_type_rule.new().call_with(["IRTokenType::Comma"]),
    virtual_variable_rule.new(),  # index
    token_type_rule.new().call_with(["IRTokenType::Comma"]),
    number_rule.new(),  # scale factor
    token_type_rule.new().call_with(["IRTokenType::Comma"]),
    number_rule.new()  # displacement
])).map_with_args(AstBuilder.memory_node.name_string, ["MemoryReferenceType::BaseIndexScaleDispl"])

direct_memory_rule = ParseRule().name("Direct").match_rule(sequence.new().match_rules([

])).map_with_args(AstBuilder.memory_node.name_string, ["MemoryReferenceType::Direct"])

index_scale_memory_rule = ParseRule().name("IndexScale").map_with_args(AstBuilder.memory_node.name_string,
                                                                       ["MemoryReferenceType::IndexScale"])

memory_node_rule = ParseRule().name("MemoryNode").match_rules([
    sequence.new().match_rules([
        token_type_rule.new().call_with(["IRTokenType::LeftParen"]),
        any_of.new().match_rules([
            base_memory_rule.new(),
            base_displ_memory_rule.new(),
            base_index_scale_displ_memory_rule.new(),
            direct_memory_rule.new(),
            index_scale_memory_rule.new()
        ]),
        token_type_rule.new().call_with(["IRTokenType::RightParen"])
    ])
])

instruction_operand_rule = ParseRule().name("InstructionOperand").match_rules([
    type_rule.new(),
    any_of.new().match_rules([
        number_rule.new(),
        virtual_variable_rule.new(),
        memory_node_rule.new()
    ])
]).map(AstBuilder.instruction_operand_node.name_string)

instruction_rule = ParseRule().name("Instruction").match_rule(sequence.new().match_rules([
    keyword_rule.new(),
    instruction_operand_rule.new(),
    optional.new().match_rules([
        many_of.new().call_with(["0", "UINT64_MAX"]).match_rules([
            token_type_rule.new().call_with(["IRTokenType::Comma"]),
            instruction_operand_rule.new()
        ])
    ])
])).map(AstBuilder.instruction_node.name_string)

module_header_rule = ParseRule().name("ModuleHeader").match_rule(sequence.new().match_rules([
    keyword_rule.new(),  # .module
    identifier_rule.new(),
    token_type_rule.new().call_with(["IRTokenType::LeftParen"]),
    many_of.new().call_with(["0", "UINT64_MAX"]).match_rule(virtual_variable_rule.new()),
    token_type_rule.new().call_with(["IRTokenType::RightParen"]),
])).map(AstBuilder.module_header_node.name_string)

module_rule = ParseRule().name("Module").match_rule(sequence.new().match_rules([
    module_header_rule.new(),
    many_of.new().call_with(["0", "UINT64_MAX"]).match_rule(instruction_rule),
    keyword_rule.new()  # .end
])).map(AstBuilder.module_node.name_string)

if __name__ == 'main':
    print("Generating grammar files")
