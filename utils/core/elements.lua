local elements = {}

function elements.mergeStyles(base, override)
	local res = {}
	for k, v in pairs(base or {}) do
		res[k] = v
	end
	for k, v in pairs(override or {}) do
		res[k] = v
	end

	return res
end

function elements.mergeChildren(listA, listB)
	local res = {}
	if listA then
		for _, child in ipairs(listA) do
			table.insert(res, child)
		end
	end

	if listB then
		for _, child in ipairs(listB) do
			table.insert(res, child)
		end
	end

	return res
end

local function createStack(typeName, props)
	props = props or {}

	local node = {
		type = typeName,
		style = props.style or {},
		children = props.children or {},
		onClick = props.onClick,
	}
	return node
end

function elements.VStack(props)
	return createStack("vstack", props)
end

function elements.HStack(props)
	return createStack("hstack", props)
end

function elements.Rect(props)
	props = props or {}

	local node = {
		type = "rect",
		style = props.style or {},
		children = nil,
		onClick = props.onClick,
	}

	return node
end

return elements
