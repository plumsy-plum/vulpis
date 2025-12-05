UI = {
	type = "rect",
	x = 100,
	y = 100,
	w = 100,
	h = 100,
	color = { 255, 0, 0 },

	children = {
		{
			type = "rect",
			x = 20,
			y = 20,
			w = 80,
			h = 40,
			color = { 0, 255, 0 },
		},
		{
			type = "rect",
			x = 40,
			y = 80,
			w = 100,
			h = 60,
			color = { 0, 0, 255 },
		},
	},
}

function render()
	drawNode(UI, 0, 0)
end
