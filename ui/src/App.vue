<script setup lang="ts">
import { ref } from 'vue'
type Module = import('wasm').Module
type LuaManager = import('wasm').LuaManager
type Window = import('wasm').Window

const execTime = ref("")

function clearOutput() {
  console.log("Clear")
  const output = document.getElementById("scriptOutput")
  if (output) {
    (output as HTMLInputElement).value = ""
  }
}

function getLua() {
  return (((window as unknown) as Window).Module as Module).LuaManager as LuaManager
}

function runCallback() {
  const lua = getLua()
  if (lua.callbackReady()) {
    const before = performance.now()
    lua.runCallback()
    const execution_time = performance.now() - before
    execTime.value = `Took ${execution_time} ms`
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

i = 0

function blinkHigh()
    i = i + 1
    print("Blink high, counter = " .. i)
    registerCallback(blinkLow)
end

function blinkLow()
    print("Blink low")
    registerCallback(blinkHigh)
end

registerCallback(blinkHigh)
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
      <button id="callbackBtn" @click="runCallback()">Run Callback</button>
      <span>{{ execTime }}</span>
    </div>
    <textarea id="scriptOutput" rows="24" cols="80"></textarea>
  </main>
</template>

<style scoped></style>
