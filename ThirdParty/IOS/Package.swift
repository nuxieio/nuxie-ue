// swift-tools-version: 6.0
import PackageDescription

let package = Package(
  name: "NuxieUnrealBridge",
  platforms: [
    .iOS(.v15),
  ],
  products: [
    .library(
      name: "NuxieUnrealBridge",
      type: .dynamic,
      targets: ["NuxieUnrealBridge"]
    ),
  ],
  dependencies: [
    .package(
      url: "https://github.com/nuxieai/nuxie-ios.git",
      exact: "0.1.0"
    ),
  ],
  targets: [
    .target(
      name: "NuxieUnrealBridge",
      dependencies: [
        .product(name: "Nuxie", package: "nuxie-ios"),
      ]
    ),
    .testTarget(
      name: "NuxieUnrealBridgeTests",
      dependencies: ["NuxieUnrealBridge"]
    ),
  ]
)
