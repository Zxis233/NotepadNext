local L = {}

L.lexer = "toml"

L.singleLineComment = "# "

L.extensions = {
	"toml",
}

L.keywords = {
	[0] = "false inf nan true",
}

-- Style IDs correspond to SCE_TOML_* in Lexilla's SciLexer.h.
L.styles = {
	["DEFAULT"] = {
		id = 0,
		fgColor = rgb(0x000000),
		bgColor = rgb(0xFFFFFF),
	},
	["COMMENT"] = {
		id = 1,
		fgColor = rgb(0x008000),
		bgColor = rgb(0xFFFFFF),
	},
	["IDENTIFIER"] = {
		id = 2,
		fgColor = rgb(0x000080),
		bgColor = rgb(0xFFFFFF),
	},
	["KEYWORD"] = {
		id = 3,
		fgColor = rgb(0x0000FF),
		bgColor = rgb(0xFFFFFF),
		fontStyle = 1,
	},
	["NUMBER"] = {
		id = 4,
		fgColor = rgb(0xFF8000),
		bgColor = rgb(0xFFFFFF),
	},
	["TABLE"] = {
		id = 5,
		fgColor = rgb(0x8000FF),
		bgColor = rgb(0xFFFFFF),
		fontStyle = 1,
	},
	["KEY"] = {
		id = 6,
		fgColor = rgb(0x000080),
		bgColor = rgb(0xFFFFFF),
	},
	["ERROR"] = {
		id = 7,
		fgColor = rgb(0xFF0000),
		bgColor = rgb(0xFFFFFF),
	},
	["OPERATOR"] = {
		id = 8,
		fgColor = rgb(0x000000),
		bgColor = rgb(0xFFFFFF),
	},
	["STRING_SQ"] = {
		id = 9,
		fgColor = rgb(0x800000),
		bgColor = rgb(0xFFFFFF),
	},
	["STRING_DQ"] = {
		id = 10,
		fgColor = rgb(0x800000),
		bgColor = rgb(0xFFFFFF),
	},
	["TRIPLE_STRING_SQ"] = {
		id = 11,
		fgColor = rgb(0x800000),
		bgColor = rgb(0xFFFFFF),
	},
	["TRIPLE_STRING_DQ"] = {
		id = 12,
		fgColor = rgb(0x800000),
		bgColor = rgb(0xFFFFFF),
	},
	["ESCAPECHAR"] = {
		id = 13,
		fgColor = rgb(0x0000FF),
		bgColor = rgb(0xFFFFFF),
		fontStyle = 1,
	},
	["DATETIME"] = {
		id = 14,
		fgColor = rgb(0x008080),
		bgColor = rgb(0xFFFFFF),
	},
	["STRINGEOL"] = {
		id = 15,
		fgColor = rgb(0xFF0000),
		bgColor = rgb(0xFFFFFF),
	},
}

return L
