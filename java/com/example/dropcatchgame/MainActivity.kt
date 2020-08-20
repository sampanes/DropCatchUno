package com.example.dropcatchgame

import android.app.ProgressDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.content.Intent
import android.os.AsyncTask
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.SystemClock
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.widget.Button
import android.widget.Chronometer
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*
import java.io.IOException
import java.util.*
import java.util.concurrent.TimeUnit

const val NUMBER_OF_STICKS:Int = 7

class MainActivity : AppCompatActivity() {

    companion object {
        var m_myUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        var m_bluetoothSocket: BluetoothSocket? = null
        lateinit var m_progress: ProgressDialog
        lateinit var m_bluetoothAdapter: BluetoothAdapter
        var m_isConnected: Boolean = false
        lateinit var m_address: String
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        m_address = intent.getStringExtra(DevActivity.EXTRA_ADDRESS)

        ConnectToDevice(this).execute()

        /*some variables*/
        var isRunning = false
        var myList = mutableListOf<String>()
        var startTime:Long = 0
        /*declare buttons*/
        val custombutton0 = findViewById<Button>(R.id.custom_button0)
        val custombutton1 = findViewById<Button>(R.id.custom_button1)
        val custombutton2 = findViewById<Button>(R.id.custom_button2)
        val custombutton3 = findViewById<Button>(R.id.custom_button3)
        val custombutton4 = findViewById<Button>(R.id.custom_button4)
        val custombutton5 = findViewById<Button>(R.id.custom_button5)
        val custombutton6 = findViewById<Button>(R.id.custom_button6)
        val resetBtn = findViewById<Button>(R.id.reset_btn)
        val senditBtn = findViewById<Button>(R.id.sendit_btn)
        /*declare chronometer*/
        val meter = findViewById<Chronometer>(R.id.mychronometer)

        /*functions*/
        fun startTheStuff(current_time: Long) {
            startTime = System.currentTimeMillis()
            mychronometer.base = SystemClock.elapsedRealtime()
            mychronometer.start()
            isRunning = true
            return
        }
        fun stopTheStuff() {
            mychronometer.stop()
            isRunning = false
            return
        }
        fun longToString(input: Long) : String {
            Toast.makeText(this, "wow " + (input%100000000).toString(),Toast.LENGTH_SHORT).show()
            var retString:String = String.format("%02d:%02d:%02d",
                TimeUnit.MILLISECONDS.toMinutes(input),
                TimeUnit.MILLISECONDS.toSeconds(input) - TimeUnit.MINUTES.toSeconds(TimeUnit.MILLISECONDS.toMinutes(input)),
                (input - TimeUnit.SECONDS.toMillis(TimeUnit.MILLISECONDS.toSeconds(input)))/10)
            return retString
        }

        /*Test Send*/
        //sendCommand("a")

        /*handle button clicks*/
        custombutton0.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("a"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("a")
            }
            it.isEnabled=false
            custombutton0.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }
        custombutton1.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("b"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("b")
            }
            it.isEnabled=false
            custombutton1.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }
        custombutton2.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("c"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("c")
            }

            it.isEnabled=false
            custombutton2.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }
        custombutton3.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("d"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("d")
            }
            it.isEnabled=false
            custombutton3.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }
        custombutton4.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("e"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("e")
            }
            it.isEnabled=false
            custombutton4.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }
        custombutton5.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("f"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("f")
            }
            it.isEnabled=false
            custombutton5.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }
        custombutton6.setOnClickListener {
            var buttonText:String="00:00:00"
            if (isRunning){
                val timeLong = System.currentTimeMillis() - startTime
                buttonText = longToString(timeLong)
                myList.add("g"+(timeLong%100000000).toString())
            } else {
                startTheStuff(System.currentTimeMillis())
                myList.add("g")
            }
            it.isEnabled=false
            custombutton6.text=buttonText
            if (myList.size == NUMBER_OF_STICKS){
                stopTheStuff()
            }
        }

        resetBtn.setOnClickListener {
            stopTheStuff()
            myList.clear()
            mychronometer.base = SystemClock.elapsedRealtime()
            custombutton0.isEnabled=true
            custombutton0.text=""
            custombutton1.isEnabled=true
            custombutton1.text=""
            custombutton2.isEnabled=true
            custombutton2.text=""
            custombutton3.isEnabled=true
            custombutton3.text=""
            custombutton4.isEnabled=true
            custombutton4.text=""
            custombutton5.isEnabled=true
            custombutton5.text=""
            custombutton6.isEnabled=true
            custombutton6.text=""

            //https://www.youtube.com/watch?v=eg-t_rhDSoM
            //sendCommand("a")
            //https://stackoverflow.com/questions/54599490/how-to-simply-pass-a-string-from-android-studio-kotlin-to-arduino-serial-bluet
            //writeData("led_on")
        }

        senditBtn.setOnClickListener {
            if (myList.isNotEmpty()) {
                mychronometer.stop()
                custombutton0.isEnabled = false
                custombutton1.isEnabled = false
                custombutton2.isEnabled = false
                custombutton3.isEnabled = false
                custombutton4.isEnabled = false
                custombutton5.isEnabled = false
                custombutton6.isEnabled = false
                Toast.makeText(applicationContext, "OK, $myList", Toast.LENGTH_LONG).show()
                //https://stackoverflow.com/questions/54599490/how-to-simply-pass-a-string-from-android-studio-kotlin-to-arduino-serial-bluet
                writeData(myList.toString())
            } else {
                Toast.makeText(this, "click at least one red button lol", Toast.LENGTH_SHORT).show()
            }
        }
        //sendit_btn.setOnClickListener { writeData("led_on") }
        //reset_btn.setOnClickListener { writeData("led_off") }

        bluetooth_btn.setOnClickListener { disconnect() }

    }

    //https://www.youtube.com/watch?v=eg-t_rhDSoM
    /*private fun sendCommand(input: String) {
        if (m_bluetoothSocket != null) {
            try {
                m_bluetoothSocket!!.outputStream.write(input.toByteArray())
            } catch (e: IOException) {
                Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show()
                e.printStackTrace()
            }
        }
    }*/
    private fun disconnect() {
        if (m_bluetoothSocket != null) {
            try {
                m_bluetoothSocket!!.close()
                m_bluetoothSocket = null
                m_isConnected = false
            } catch (e: IOException) {
                Toast.makeText(this, e.toString(), Toast.LENGTH_SHORT).show()
                e.printStackTrace()
            }
        }
        finish()
    }
    private class ConnectToDevice(c: Context) : AsyncTask<Void, Void, String>() {
        private var connectSuccess: Boolean = true
        private val context: Context

        init {
           this.context = c
        }

        override fun onPreExecute() {
            super.onPreExecute()
            m_progress = ProgressDialog.show(context, "Connecting...", "please wait")
        }

        override fun doInBackground(vararg params: Void?): String? {
            try{
                if (m_bluetoothSocket == null || !m_isConnected) {
                    m_bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
                    val device: BluetoothDevice = m_bluetoothAdapter.getRemoteDevice(m_address)
                    m_bluetoothSocket = device.createInsecureRfcommSocketToServiceRecord(m_myUUID)
                    BluetoothAdapter.getDefaultAdapter().cancelDiscovery()
                    m_bluetoothSocket!!.connect()
                }
            } catch (e: IOException) {
                connectSuccess = false
                e.printStackTrace()
            }
            return null
        }

        override fun onPostExecute(result: String?) {
            super.onPostExecute(result)
            if (!connectSuccess) {
                Log.i("data", "couldn't connect")
            } else {
                m_isConnected = true
            }
            m_progress.dismiss()
        }
    }

    //https://stackoverflow.com/questions/54599490/how-to-simply-pass-a-string-from-android-studio-kotlin-to-arduino-serial-bluet
    private fun writeData(data: String) {
        var outStream = m_bluetoothSocket?.outputStream//var outStream = m_bluetoothSocket.outputStream
        try {
            outStream = m_bluetoothSocket?.outputStream//outStream = m_bluetoothSocket.outputStream
        } catch (e: IOException) {
            //Log.d(FragmentActivity.TAG, "Bug BEFORE Sending stuff", e)
        }
        val msgBuffer = data.toByteArray()

        try {
            outStream?.write(msgBuffer)//outStream.write(msgBuffer)
        } catch (e: IOException) {
            //Log.d(FragmentActivity.TAG, "Bug while sending stuff", e)
        }

    }

    //https://www.youtube.com/watch?v=wMGlat24Zmw
    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        super.onCreateOptionsMenu(menu)
        menuInflater.inflate(R.menu.main,menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {

        var selectedOption = ""

        when(item?.itemId){
            R.id.dev_options -> selectedOption = "Dev Options"
            R.id.get_help -> selectedOption = "Get Help"
        }

        if (selectedOption == "Dev Options"){
            val intent = Intent(this, DevActivity::class.java)
        }

        return super.onOptionsItemSelected(item)
    }
}
//https://www.youtube.com/watch?v=Oz4CBHrxMMs&t=1891s this is the kotlin bluetooth part 1 discusses switching activities