import 'package:intl/intl.dart';

abstract final class UnitFormatter {
  static final _oneDecimal = NumberFormat('0.0', 'ru');
  static final _twoDecimals = NumberFormat('0.00', 'ru');

  static String speed(int speedX100, {required bool imperial}) {
    final kmh = speedX100 / 100;
    final value = imperial ? kmh * 0.621371 : kmh;
    return '${_oneDecimal.format(value)} ${imperial ? 'mph' : 'км/ч'}';
  }

  static String distanceCm(int distanceCm, {required bool imperial}) {
    final km = distanceCm / 100000;
    final value = imperial ? km * 0.621371 : km;
    return '${_twoDecimals.format(value)} ${imperial ? 'mi' : 'км'}';
  }

  static String odometerM(int meters, {required bool imperial}) {
    final km = meters / 1000;
    final value = imperial ? km * 0.621371 : km;
    return '${_oneDecimal.format(value)} ${imperial ? 'mi' : 'км'}';
  }

  static String duration(int seconds) {
    final hours = seconds ~/ 3600;
    final minutes = (seconds % 3600) ~/ 60;
    final remainder = seconds % 60;
    return '${hours.toString().padLeft(2, '0')}:'
        '${minutes.toString().padLeft(2, '0')}:'
        '${remainder.toString().padLeft(2, '0')}';
  }
}
