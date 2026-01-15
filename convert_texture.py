from PIL import Image

# 打开webp文件
img = Image.open('assets/OIP.webp')

# 调整大小为16x16，使用LANCZOS重采样以获得更好的质量
img_resized = img.resize((16, 16), Image.Resampling.LANCZOS)

# 确保是RGBA格式
if img_resized.mode != 'RGBA':
    img_resized = img_resized.convert('RGBA')

# 保存为PNG
img_resized.save('assets/texture.png')
print("成功将OIP.webp转换为16x16的texture.png")
