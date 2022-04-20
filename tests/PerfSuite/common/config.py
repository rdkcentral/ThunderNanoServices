import json


class Config:
    def __init__(self, filePath):
        self.filePath = filePath
        self.data = {}
        self.load_config()

    def load_config(self):
        with open(self.filePath) as json_file:
            self.data = json.load(json_file)

    def get_config(self, key, value):
        for d in self.data["dutLists"]:
            if key in d and d[key] == value:
                return d

    def get_types(self):
        types = []
        for d in self.data["dutLists"]:
            if "type" in d:
                types.append(d["type"])
        return types
