const HTMLInlineCSSWebpackPlugin =
  require("html-inline-css-webpack-plugin").default;
const HtmlInlineScriptPlugin = require("html-inline-script-webpack-plugin");

plugins: [
  isEnvProduction &&
    shouldInlineRuntimeChunk &&
    new HTMLInlineCSSWebpackPlugin(),
  isEnvProduction && shouldInlineRuntimeChunk && new HtmlInlineScriptPlugin(),
];
