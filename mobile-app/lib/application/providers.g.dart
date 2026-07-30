// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'providers.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(bleTransport)
final bleTransportProvider = BleTransportProvider._();

final class BleTransportProvider
    extends $FunctionalProvider<BleTransport, BleTransport, BleTransport>
    with $Provider<BleTransport> {
  BleTransportProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'bleTransportProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$bleTransportHash();

  @$internal
  @override
  $ProviderElement<BleTransport> $createElement($ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  BleTransport create(Ref ref) {
    return bleTransport(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(BleTransport value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<BleTransport>(value),
    );
  }
}

String _$bleTransportHash() => r'fdb4665178af2323750798072f2c6c4ca5227c16';

@ProviderFor(preferencesStore)
final preferencesStoreProvider = PreferencesStoreProvider._();

final class PreferencesStoreProvider
    extends
        $FunctionalProvider<
          PreferencesStore,
          PreferencesStore,
          PreferencesStore
        >
    with $Provider<PreferencesStore> {
  PreferencesStoreProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'preferencesStoreProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$preferencesStoreHash();

  @$internal
  @override
  $ProviderElement<PreferencesStore> $createElement($ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  PreferencesStore create(Ref ref) {
    return preferencesStore(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(PreferencesStore value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<PreferencesStore>(value),
    );
  }
}

String _$preferencesStoreHash() => r'308c8786018bbd06776779b670a2c673413b45f4';

@ProviderFor(androidBlePlatform)
final androidBlePlatformProvider = AndroidBlePlatformProvider._();

final class AndroidBlePlatformProvider
    extends
        $FunctionalProvider<
          AndroidBlePlatform,
          AndroidBlePlatform,
          AndroidBlePlatform
        >
    with $Provider<AndroidBlePlatform> {
  AndroidBlePlatformProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'androidBlePlatformProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$androidBlePlatformHash();

  @$internal
  @override
  $ProviderElement<AndroidBlePlatform> $createElement(
    $ProviderPointer pointer,
  ) => $ProviderElement(pointer);

  @override
  AndroidBlePlatform create(Ref ref) {
    return androidBlePlatform(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(AndroidBlePlatform value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<AndroidBlePlatform>(value),
    );
  }
}

String _$androidBlePlatformHash() =>
    r'c4f5a4b158ca91775ba9bc26554e11be2575f956';

@ProviderFor(ConfigDraftController)
final configDraftControllerProvider = ConfigDraftControllerProvider._();

final class ConfigDraftControllerProvider
    extends $NotifierProvider<ConfigDraftController, ConfigDraftState> {
  ConfigDraftControllerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'configDraftControllerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$configDraftControllerHash();

  @$internal
  @override
  ConfigDraftController create() => ConfigDraftController();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(ConfigDraftState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<ConfigDraftState>(value),
    );
  }
}

String _$configDraftControllerHash() =>
    r'321bb0f6e378ab00f38d95c61d247b2650865f45';

abstract class _$ConfigDraftController extends $Notifier<ConfigDraftState> {
  ConfigDraftState build();
  @$mustCallSuper
  @override
  WhenComplete runBuild() {
    final ref = this.ref as $Ref<ConfigDraftState, ConfigDraftState>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<ConfigDraftState, ConfigDraftState>,
              ConfigDraftState,
              Object?,
              Object?
            >;
    return element.handleCreate(ref, build);
  }
}

@ProviderFor(ConnectionController)
final connectionControllerProvider = ConnectionControllerProvider._();

final class ConnectionControllerProvider
    extends $NotifierProvider<ConnectionController, SessionState> {
  ConnectionControllerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'connectionControllerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$connectionControllerHash();

  @$internal
  @override
  ConnectionController create() => ConnectionController();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(SessionState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<SessionState>(value),
    );
  }
}

String _$connectionControllerHash() =>
    r'18530b75f82cb74975f1f5ae47027dbf97039559';

abstract class _$ConnectionController extends $Notifier<SessionState> {
  SessionState build();
  @$mustCallSuper
  @override
  WhenComplete runBuild() {
    final ref = this.ref as $Ref<SessionState, SessionState>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<SessionState, SessionState>,
              SessionState,
              Object?,
              Object?
            >;
    return element.handleCreate(ref, build);
  }
}
