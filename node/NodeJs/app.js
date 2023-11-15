const colorflow = require("./build/Release/colorflow");

process.stdin.resume();
process.stdin.setEncoding('utf8');

process.stdin.on('data', function (image_name) {
  let pathToImg =  "../C-project/pictures/"+image_name;
  pathToImg = pathToImg.slice(0, -1);
  console.log(colorflow.getAverageColor(pathToImg));
});
