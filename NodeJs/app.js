const colorflow = require("./build/Release/colorflow");

var args = process.argv.slice(2);
try{
  console.log(colorflow.getAverageColor("../pictures/"+args[0]));
}
catch (err){
  console.error("Error: app.js needs argument");
}
