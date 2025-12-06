UI = {
	type = "vstack",
	padding = 5,
	children = {
		{
			type = "rect",
			w = 800,
			h = 80,
			margin = 10,
			color = { 255, 0, 0 },
		},
		{
			type = "rect",
			w = 200,
			h = 200,
			margin = 10,
			color = { 0, 255, 0 },
		},
		{
			type = "hstack",
			spacing = 25,
			margin = 10,

			children = {
				{ type = "rect", w = 50, h = 50, color = { 200, 200, 0 } },
				{ type = "rect", w = 50, h = 50, color = { 0, 200, 200 } },
				{ type = "rect", w = 50, h = 50, color = { 200, 0, 200 } },
			},
		},
	},
}
