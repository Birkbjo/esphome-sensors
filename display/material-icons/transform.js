const fs = require("fs");
const path = require("path");
const sharp = require("sharp");
const yaml = require("yaml");

const isUnixHiddenPath = function (path) {
    return /(^|\/)\.[^\/\.]/g.test(path);
};

const convertToPng = (srcFolder, name, dest) => {
    const withoutExt = name
        .replace("weather-", "")
        .replace("-", "_")
        .split(".")[0];
    const output = dest
        ? path.join(dest, `${withoutExt}.png`)
        : `${withoutExt}.png`;

    if (fs.existsSync(output)) {
        return [output];
    }

    const result = sharp(path.join(srcFolder, name), { density: 500 })
        .flatten({ background: "#ffffff" })
        .resize({ width: 100 })
        .sharpen()
        .png()
        .toFile(output);

    return [output, result];
};

const generateEspHomeYaml = (imagePaths, outFolder, imageOpts) => {
    const doc = new yaml.Document();

    const names = imagePaths.map(([p]) => p.match(/\/(.+)$/)[1]);

    console.log(names);
    let str = "";
    
    // generate c++ map to get hold of variables for images
    // this is very hacky, but esphome have no way to have dynamic images
    names
        .map((n) => n.split(".")[0])
        .forEach((n) => (str += `images["${n}"] = ${n};\n`));

    console.log(str)
    const yamlStruct = names.map((n) => ({
        ...imageOpts,
        file: path.join(outFolder, n),
        id: n.split(".")[0],
    }));

    doc.contents = {
        image: yamlStruct,
    };

    return yaml.stringify(doc);
};

const main = async () => {
    const readFolder = "original";
    const writeFolder = "pngs";

    const files = fs
        .readdirSync(readFolder, { withFileTypes: true })
        .filter((dirent) => dirent.isFile && !isUnixHiddenPath(dirent.name))
        .map((d) => d.name);

    console.log("Transforming", files);
    const promises = files.map((f) => convertToPng(readFolder, f, writeFolder));

    const names = await Promise.all(promises);

    const outFolder = path.join("material-icons", writeFolder);
    const yaml = generateEspHomeYaml(names, outFolder, {
        resize: "70x70",
    });

    console.log(yaml);
};

main();
