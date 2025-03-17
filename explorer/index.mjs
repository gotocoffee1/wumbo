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
editorWat.setReadOnly(true);
initEditor(editorWat);

import { newInstance } from "./wumbo.mjs";

const optimize = document.getElementById("optimize");
const watFormat = document.getElementById("watFormat");

run.onclick = async () => {
  editorConsole.setValue("");
  editorWat.setValue("");
  try {
    const load = await newInstance({
      override: {
        print: {
          string: (arg) => {
            arg = new TextDecoder().decode(arg);
            console.log(arg);
            appendConsole(arg);
          },
        },
      },
      optimize: optimize.checked,
      format: watFormat.checked,
      standalone: false,
    });
    const [f, wat] = await load(editorLua.getValue());
    editorWat.setValue(wat);
    f();
  } catch (e) {
    console.error(e);
    appendConsole(e);
  }
};
