local path_sep = require("catppuccin").path_sep
local O = require("catppuccin").options
local M = {}

-- Credit: https://github.com/EdenEast/nightfox.nvim
local fmt = string.format

local function inspect(t)
	local list = {}
	for k, v in pairs(t) do
		local tv = type(v)
		if tv == "string" then
			table.insert(list, fmt([[.%s = s("%s")]], k, v))
		elseif tv == "boolean" then
			table.insert(list, fmt([[.%s = %s]], k, v and "True" or "False"))
		elseif tv == "table" then
			table.insert(list, fmt([[.%s = %s]], k, inspect(v)))
		else
			table.insert(list, fmt([[.%s = %s]], k, tostring(v)))
		end
	end
	table.sort(list)
	return table.concat(list, ", ")
end

function M.compile(flavour)
	local theme = require("catppuccin.lib.mapper").apply(flavour)
	local lines = {
		string.format(
			[[
#include "nvim.h"
int luaopen_catppuccin_compiled_%s(lua_State *lstate) {
	o("termguicolors", True);
	g("colors_name", s("catpussy"));
	Error err = ERROR_INIT;
	KeyDict_highlight o;
]],
			flavour
		),
	}
	table.insert(lines, string.format([[o("background", s("%s"));]], (flavour == "latte" and "light" or "dark")))
	if path_sep == "\\" then O.compile_path = O.compile_path:gsub("/", "\\") end
	O.compile_path = debug.getinfo(1).source:sub(2, -10) .. "compiled"

	local tbl = vim.tbl_deep_extend("keep", theme.custom_highlights, theme.integrations, theme.syntax, theme.editor)

	if O.term_colors == true then
		for k, v in pairs(theme.terminal) do
			table.insert(lines, fmt('g("%s", s("%s"));', k, v))
		end
	end

	local groups = {}
	for group, color in pairs(tbl) do
		if color.style then
			for _, style in pairs(color.style) do
				color[style] = true
				if O.no_italic and style == "italic" then color[style] = false end
				if O.no_bold and style == "bold" then color[style] = false end
			end
		end
		color.style = nil
		if color.link and (theme.custom_highlights[group] and not theme.custom_highlights[group].link) then
			color.link = nil
		end
		-- table.insert(lines, fmt([[h("%s", %s);]], group, inspect(color)))
		local opts = inspect(color)
		groups[opts] = groups[opts] or {}
		table.insert(groups[opts], group)
	end
	for opts, group in pairs(groups) do
		table.sort(group, function(a, b) return #a < #b end)
		if #group == 1 then
			table.insert(lines, fmt([[h("%s", %s);]], group[1], opts))
		else
			for i, v in ipairs(group) do
				if i == 1 then table.insert(lines, fmt([[o = (KeyDict_highlight){ %s };]], opts, v)) end
				table.insert(lines, fmt([[c("%s");]], v, i))
			end
			table.insert(lines, [[api_free_keydict_highlight(&o);]])
		end
	end
	table.insert(lines, "Error_check\n}")
	if vim.fn.isdirectory(O.compile_path) == 0 then vim.fn.mkdir(O.compile_path, "p") end

	local file = io.open(O.compile_path .. path_sep .. flavour .. ".c", "wb")
	file:write(table.concat(lines, "\n"))
	file:close()
end

return M
