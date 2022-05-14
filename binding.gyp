{
	"targets": [
		{
			"target_name": "cloudwatch-nodejs-plugin",
			"sources": [
				"src/nativeStats.cc"
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			],
		},
		{
			"target_name": "action_after_build",
			"type": "none",
			"dependencies": ["cloudwatch-nodejs-plugin"],
			"copies": [
				{
					"files": [
						"./src/README.md",
						"./build/Release/cloudwatch-nodejs-plugin.node",
						"./package.json"
					],
					"destination": "./cloudwatch-nodejs-plugin/"
				}
			]
		}
	]
}
