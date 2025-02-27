import ace from "https://cdn.jsdelivr.net/npm/ace-builds@1.38.0/+esm";

ace.config.set(
  "basePath",
  "https://cdn.jsdelivr.net/npm/ace-builds@1.38.0/src-min-noconflict",
);

const initEditor = (editor) => {
  const theme = "ace/theme/gruvbox";
  editor.setTheme(theme);
  editor.setOption("showPrintMargin", false);
};

const editorLua = ace.edit("editor-lua");
editorLua.session.setMode("ace/mode/lua");
initEditor(editorLua);

const editorConsole = ace.edit("editor-console");
editorConsole.setReadOnly(true);
initEditor(editorConsole);
const session = editorConsole.session;
const appendConsole = (arg) => {
  session.insert(
    {
      row: session.getLength(),
      column: 0,
    },
    arg + "\n",
  );
};

const editorWat = ace.edit("editor-wat");
initEditor(editorWat);

import { newInstance } from "./wumbo.mjs";
const load = await newInstance({
  print: {
    string: (arg) => {
      arg = new TextDecoder().decode(arg);
      console.log(arg);
      appendConsole(arg);
    },
  },
});
run.onclick = async () => {
  editorConsole.setValue("");
  try {
    const [f, wat] = await load(editorLua.getValue());
    editorWat.setValue(wat);
    f();
  } catch (e) {
    console.error(e);
    appendConsole(e);
  }
};
