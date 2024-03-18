from PIL import Image


def create_image_from_data(data):
    # 定义颜色映射函数
    def map_value_to_color(value):
        # 线性映射到0-255范围
        if value > 1000:
            value = 1000
        return int(value * 255 / 1000)

    # 创建新的图像
    width, height = len(data[0]), len(data)
    image = Image.new('RGB', (width, height), 'white')
    pixels = image.load()

    # 遍历数据并填充像素
    for y in range(height):
        for x in range(width):
            # 映射值到灰度值
            grayscale_value = 255 - map_value_to_color(data[y][x])
            # 设置像素颜色
            pixels[x, y] = (grayscale_value, grayscale_value, grayscale_value)

    return image


# 读取数据，这里假设你有一个名为 "data.txt" 的文件
def read_data_from_file(filename):
    data = []
    with open(filename, 'r') as file:
        for line in file:
            line_data = list(map(int, line.split()))  # 将每行的数字转换为整数列表
            data.append(line_data)
    return data


for i in range(0, 10):
    # 读取数字数据
    data = read_data_from_file(str(i))

    # 创建图像
    image = create_image_from_data(data)

    # 保存图像
    image.save("output_image" + str(i) + ".png")
    print("Image saved as output_image.png")
