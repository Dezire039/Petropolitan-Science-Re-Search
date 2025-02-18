#include <iostream>
#include <vector>
#include <cmath>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// Функция для определения угла поворота изображения
double detectRotationAngle(const Mat& image) {
    // 1. Предобработка
    Mat gray, blurred, thresholded;
    cvtColor(image, gray, COLOR_BGR2GRAY); // Преобразование в градации серого
    GaussianBlur(gray, blurred, Size(5, 5), 0); // Размытие

    adaptiveThreshold(blurred, thresholded, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 11, 2);  // Адаптивный порог

    // 2. Обнаружение линий (Hough Transform)
    vector<Vec2f> lines;
    HoughLines(thresholded, lines, 1, CV_PI / 180, 100, 50, 10);  // Настройка параметров

    // 3. Определение угла
    vector<double> angles;
    for (size_t i = 0; i < lines.size(); i++) {
        float rho = lines[i][0], theta = lines[i][1];
        double angle = theta * 180 / CV_PI;  // Convert to degrees

        // Отбрасываем вертикальные линии и линии, углы которых выходят за пределы разумного
        if (abs(angle) > 5 && abs(angle) < 175) //Если нужна фильтрация, можно менять
            angles.push_back(angle > 90 ? angle - 180 : angle);
    }

    // Вычисление медианного угла
    if (angles.empty()) return 0.0; //Если линий не найдено

    sort(angles.begin(), angles.end());
    double medianAngle;
    size_t middle = angles.size() / 2;
    if (angles.size() % 2 == 0) {
        medianAngle = (angles[middle - 1] + angles[middle]) / 2.0;
    }
    else {
        medianAngle = angles[middle];
    }

    return medianAngle;
}

// Функция для поворота изображения
Mat rotateImage(const Mat& image, double angle) {
    Point2f center(image.cols / 2.0, image.rows / 2.0);
    Mat rotationMatrix = getRotationMatrix2D(center, angle, 1.0);
    Mat rotatedImage;
    warpAffine(image, rotatedImage, rotationMatrix, image.size());
    return rotatedImage;
}

int main() {
    // 1. Загрузка изображения
    Mat image = imread("document.jpg");  // Замените на имя вашего файла
    if (image.empty()) {
        cerr << "Could not open or find the image" << endl;
        return -1;
    }

    // 2. Определение угла поворота
    double angle = detectRotationAngle(image);
    cout << "Detected rotation angle: " << angle << endl;

    // 3. Коррекция изображения
    Mat rotatedImage = rotateImage(image, angle);

    // 4. Отображение и сохранение результатов
    imshow("Original Image", image);
    imshow("Rotated Image", rotatedImage);
    imwrite("rotated_document.jpg", rotatedImage);
    waitKey(0);

    return 0;
}