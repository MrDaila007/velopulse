package app.bikecomp.mobile

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.Build
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
  override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    MethodChannel(
      flutterEngine.dartExecutor.binaryMessenger,
      "app.bikecomp.mobile/ble_platform",
    ).setMethodCallHandler { call, result ->
      when (call.method) {
        "requestEnableBluetooth" -> {
          startActivity(Intent(Settings.ACTION_BLUETOOTH_SETTINGS))
          result.success(true)
        }
        "sdkInt" -> result.success(Build.VERSION.SDK_INT)
        "locationServicesEnabled" -> result.success(locationServicesEnabled())
        "bondState" -> result.success(bondState(call.argument<String>("deviceId")))
        else -> result.notImplemented()
      }
    }
  }

  private fun locationServicesEnabled(): Boolean {
    val manager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      manager.isLocationEnabled
    } else {
      @Suppress("DEPRECATION")
      manager.isProviderEnabled(LocationManager.GPS_PROVIDER) ||
        manager.isProviderEnabled(LocationManager.NETWORK_PROVIDER)
    }
  }

  private fun bondState(deviceId: String?): String {
    if (deviceId.isNullOrBlank()) return "unknown"
    if (
      Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
      checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED
    ) return "unknown"

    return try {
      val manager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
      val adapter: BluetoothAdapter = manager.adapter ?: return "unknown"
      val device = adapter.bondedDevices.firstOrNull {
        it.address.equals(deviceId, ignoreCase = true)
      }
      if (device?.bondState == BluetoothDevice.BOND_BONDED) "bonded" else "none"
    } catch (_: IllegalArgumentException) {
      "unknown"
    } catch (_: SecurityException) {
      "unknown"
    }
  }
}
