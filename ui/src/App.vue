<script setup lang="ts">
import { ref } from 'vue'
type Window = import('wasm').Window

const execTime = ref("")
const timeNow = ref(0)
const schedulerRunning = ref(false)

function clearOutput() {
  console.log("Clear")
  const output = document.getElementById("scriptOutput")
  if (output) {
    (output as HTMLInputElement).value = ""
  }
}

function getWindow() {
  return ((window as unknown) as Window)
}

function startScheduler() {
  schedulerRunning.value = true
  setTimeout(schedulerTimeout, 10)
}

function stopScheduler() {
  schedulerRunning.value = false
}

function schedulerTimeout() {
  stepScheduler()
  if (schedulerRunning.value === true) {
    setTimeout(schedulerTimeout, 1)
  }
}

function stepScheduler() {
  const lua = getWindow().Module.LuaManager
  const before = performance.now()
  lua.runScheduler(timeNow.value)
  const execution_time = performance.now() - before
  execTime.value = ` - Took ${execution_time} ms `
  timeNow.value = timeNow.value + 10
  getWindow().Module.print("")
}

</script>

<script lang="ts">
const Module: any = {
  print: function (t: string) {
    const text = `${t}\n`
    console.log(text)
    const output = document.getElementById("scriptOutput")
    if (output) {
      (output as HTMLInputElement).value += text
      if ((output as HTMLInputElement).value.length > 1000) {
        (output as HTMLInputElement).value = (output as HTMLInputElement).value.slice(-1000)
      }
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

function blinkHigh(now)
    i = i + 1
    print("Thread " .. THREAD_IDX .. " blink high, counter = " .. i)
    continueAt(now + 10, blinkLow)
end

function blinkLow(now)
    print("Thread " .. THREAD_IDX .. " blink low")
    continueAt(now + 10, blinkHigh)
end

createThread("thread1", blinkHigh, "blinkHigh")
createThread("thread2", blinkLow, "blinkLow")
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
      <button id="startBtn" @click="startScheduler()">Start Scheduler</button>
      <button id="stopBtn" @click="stopScheduler()">Stop Scheduler</button>
      <button id="stepBtn" @click="stepScheduler()">Step Scheduler</button>
    </div>
    <div>
      <span>Now: {{ timeNow }} </span>
      <span>{{ execTime }}</span>
    </div>
    <textarea id="scriptOutput" rows="24" cols="80"></textarea>
  </main>
</template>

<style scoped></style>
