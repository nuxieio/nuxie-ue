import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
  id("com.android.library")
  id("org.jetbrains.kotlin.android")
}

android {
  namespace = "ai.nuxie.unreal"
  compileSdk = 36

  defaultConfig {
    minSdk = 23
  }

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_17
    targetCompatibility = JavaVersion.VERSION_17
  }

  kotlin {
    compilerOptions {
      jvmTarget.set(JvmTarget.JVM_17)
    }
  }

  testOptions {
    unitTests.isReturnDefaultValues = true
  }
}

dependencies {
  implementation("ai.nuxie:nuxie-android:0.1.0")
  implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.9.0")
  testImplementation("junit:junit:4.13.2")
}

val preparedAar = rootProject.layout.projectDirectory.file(
  "lib/nuxie-unreal-bridge.aar",
)

tasks.register<Copy>("prepareBridgeAar") {
  dependsOn("assembleRelease")
  from(layout.buildDirectory.file("outputs/aar/bridge-release.aar"))
  into(preparedAar.asFile.parentFile)
  rename { preparedAar.asFile.name }
}
