# Native list to webp

## 简介

这是一个简单的从 concat 脚本中读取各帧并将这些帧转换为 webp 格式的动图的库,
Android 专用。

## 使用方法

build.gradle 文件中加入如下内容

```text
implementation "io.github.tobelogin:native-list2webp:0.2"
```

唯一 API: 

```java
import io.github.tobelogin.FormatConverter;

FormatConverter.list2webp(listPath, webpPath);
```

## 构建

```shell
# 构建 aar 包
./gradlew assembleRelease

# 发布到本地仓库
./gradlew publishToMavenLocal
```

## 致谢

[ffmpeg](https://ffmpeg.org)

[libwebp](https://chromium.googlesource.com/webm/libwebp)