<script setup lang="ts">
function clearOutput() {
  console.log("Clear")
  const output = document.getElementById("scriptOutput")
  if (output) {
    (output as HTMLInputElement).value = ""
  }
}
</script>

<script lang="ts">
const Module: any = {
  print: function (t: string) {
    const text = t + "\n"
    console.log(text);
    const output = document.getElementById("scriptOutput")
    if (output) {
      (output as HTMLInputElement).value += text
      output.scrollTop = output.scrollHeight
    }
  }
}
window[("Module" as any)] = Module

const doString = () => {
  const input = document.getElementById("scriptInput")
  if (input) {
    Module.LuaManager.doString((input as HTMLInputElement).value)
  }
}
export default {

  mounted() {
    const wasmLoadScript = document.createElement("script")
    wasmLoadScript.setAttribute("src", "./webbed.js")
    document.head.appendChild(wasmLoadScript)

    const runBtn = document.getElementById("runBtn")
    if (runBtn) {
      runBtn.addEventListener("click", doString)
    }
    const input = document.getElementById("scriptInput")
    if (input) {
      (input as HTMLInputElement).value = `print("Hello from Lua!")

for i = 1,5 do
  print('i = ' .. i)
end
`
    }
  },

}
</script>

<template>
  <main>
    <h1>
      Lua Scripting on WASM Demo
    </h1>
    <textarea id="scriptInput" rows="24" cols="80"></textarea>
    <div>
      <button id="runBtn">Run Script</button>
      <button id="clearBtn" @click="clearOutput()">Clear Output</button>
    </div>
    <textarea id="scriptOutput" rows="24" cols="80"></textarea>
  </main>
</template>

<style scoped></style>
