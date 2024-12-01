#include <GLUT/glut.h>  //OpenGL
#include <cmath>

// カメラの初期位置と注視点
float eyeX = 0.0f, eyeY = 0.0f, eyeZ = 5.0f;
float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
float upX = 0.0f, upY = 1.0f, upZ = 0.0f;

// 回転角度
float angle = 0.0f;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 回転角度を使って注視点を計算
    float radius = 5.0f; // カメラと注視点の距離
    centerX = eyeX + radius * sin(angle);
    centerZ = eyeZ - radius * cos(angle); // OpenGLのZ軸は奥行き

    // gluLookAtでカメラを設定
    gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

    // シーンの描画
    glutWireTeapot(1.0); // ワイヤーフレームのティーポットを描画

    glutSwapBuffers();
}

void update(int value) {
    angle += 0.01f; // 回転速度
    if (angle > 2 * M_PI) {
        angle -= 2 * M_PI; // 角度のリセット
    }
    glutPostRedisplay(); // 再描画
    glutTimerFunc(16, update, 0); // 約60FPSで更新
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Camera Rotation");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
