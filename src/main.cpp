#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <chrono>    // std::chrono

static void help();
void CornerSort( std::vector<cv::Point2f> &corners, cv::Size img_in_size);
cv::Mat DrawLines(const cv::Mat img, std::vector<cv::Point2f> corners, bool use_color);
bool JudgeSuccess(const std::pair<int, int> &pred, const std::pair<int, int> &GT, const int eps);
std::vector<cv::Point2f> CornerTrans(cv::Size img_tmp_size, cv::Mat h);
void OutputProjectionTrans(const cv::Mat img_tmp, const cv::Mat img_in, cv::Mat h, bool use_color);
void OutputRect(std::vector<cv::Point2f> dst_corners, cv::Size img_tmp_size, cv::Mat img_in, bool use_color);
void OutputAllMatch(cv::Mat img_tmp, std::vector<cv::KeyPoint> keyImg_tmp, cv::Mat img_in, std::vector<cv::KeyPoint> keyImg_in, std::vector<cv::DMatch> bestMatches);
void OutputInlierMatch(cv::Mat img_tmp, std::vector<cv::KeyPoint> keyImg_tmp, cv::Mat img_in, std::vector<cv::KeyPoint> keyImg_in, std::vector<cv::DMatch> bestMatches, cv::Mat mask);

int main(int argc, char *argv[]){
    int best_match_size = 30;
    bool use_time_stamp = false;
    bool use_color = cv::IMREAD_GRAYSCALE; // = 0
    bool use_project, use_rect, use_all, use_inlier;
    use_project = use_rect = use_all = use_inlier = false;
    double msec;
    std::chrono::system_clock::time_point start, end;

    /*=================================================================*/
    cv::CommandLineParser parser(argc, argv,
        "{ @tmp | data/template.png | Template image.}"
        "{ @input | data/img_0.png | Input image.}"
        "{ best_match_size | | Number of good response points.}"
        "{ use_color | |}"
        "{ use_project | | Drawing a projective transformed template image.}"
        "{ use_rect | | Drawing a rectangular transformed template image.}"
        "{ use_all | | Drawing corresponding points with small distances.}"
        "{ use_inlier | | Drawing only inliers.}"
        "{ time t || To measure processing time.}"
        "{ help h ||}");

    if (parser.has("help")){
        help();
        return 0;
    }
    best_match_size = parser.get<int>("best_match_size");
    use_color = parser.has("use_color");
    use_time_stamp = parser.has("time");
    use_project = parser.has("use_project");
    use_rect = parser.has("use_rect");
    use_all = parser.has("use_all");
    use_inlier = parser.has("use_inlier");

    /*=================================================================*/
    cv::Mat img_tmp = cv::imread(parser.get<cv::String>("@tmp"), use_color);
    if (img_tmp.empty()){
        std::cout << "Image " << parser.get<cv::String>("@tmp") << " is empty or cannot be found\n";
        return 0;
    }
    cv::Mat img_in = cv::imread(parser.get<cv::String>("@input"), use_color);
    if (img_in.empty()){
        std::cout << "Image " << parser.get<cv::String>("@input") << " is empty or cannot be found\n";
        return 0;
    }

    std::cout << "**************************************************************************\n";
    std::cout << "**                               AKAZE                                  **\n";
    std::cout << "**************************************************************************\n";

    /*=================================================================*/
    // AKAZE
    cv::Ptr<cv::Feature2D> akaze = cv::AKAZE::create();
    cv::Ptr<cv::DescriptorMatcher> descriptorMatcher = cv::DescriptorMatcher::create("BruteForce");
    std::vector<cv::KeyPoint> keyImg_tmp, keyImg_in; // keypoint  for img1 and img2
    cv::Mat descImg_tmp, descImg_in; // Descriptor for img1 and img2
    std::vector<cv::DMatch> matches; // Match between img1 and img2
    
    akaze->detect(img_tmp, keyImg_tmp); // We can detect keypoint with detect method
    akaze->compute(img_tmp, keyImg_tmp, descImg_tmp); // and compute their descriptors with method compute
    //akaze->detectAndCompute(img_tmp, cv::Mat(), keyImg_tmp, descImg_tmp, false); // or detect and compute descriptors in one step
    
    if(use_time_stamp)  start = std::chrono::system_clock::now();

    akaze->detect(img_in, keyImg_in);
    akaze->compute(img_in, keyImg_in, descImg_in);
    //akaze->detectAndCompute(img_in, cv::Mat(),keyImg_in, descImg_in,false);

    descriptorMatcher->match(descImg_tmp, descImg_in, matches); // 引数の順番注意

    // Keep best matches only to have a nice drawing.
    // We sort distance between descriptor matches
    cv::Mat tab(int(matches.size()), 1, CV_32F); // = matches.size() * 1 (float)
    cv::Mat index;
    for (int i = 0; i < int(matches.size()); i++) tab.at<float>(i, 0) = matches[i].distance;
    cv::sortIdx(tab, index, cv::SORT_EVERY_COLUMN + cv::SORT_ASCENDING); // Sort columns in ascending order
    std::cout << "Match size: " << matches.size() << "\n";
    if((int)matches.size() < best_match_size){
        std::cout << "Best match size: " << best_match_size << " -> " << matches.size() << "\n";
        best_match_size = (int)matches.size();
    }
    else    std::cout << "Best match size: " << best_match_size << "\n";
    std::vector<cv::DMatch> bestMatches;
    for (int i = 0; i < best_match_size; i++)  bestMatches.push_back(matches[index.at<int>(i, 0)]);
    
    /*=================================================================*/
    if(use_time_stamp){
	end = std::chrono::system_clock::now();
	msec = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() * 0.001;
        std::cout << "Time: " << msec << "[ms]\n";
    }
    /*=================================================================*/
    if(use_project || use_rect){
        std::vector<cv::KeyPoint> bestMatches_tmp, bestMatches_in;
        std::vector<cv::Point2f> bestPoint_tmp, bestPoint_in;
        for(int i = 0; i < best_match_size; i++){
            bestMatches_tmp.push_back(keyImg_tmp[bestMatches.at(i).queryIdx]);
            bestMatches_in.push_back(keyImg_in[bestMatches.at(i).trainIdx]);
        }
        cv::KeyPoint::convert(bestMatches_tmp, bestPoint_tmp);
        cv::KeyPoint::convert(bestMatches_in, bestPoint_in);
        cv::Mat mask;
        cv::Mat h = cv::findHomography(bestPoint_tmp, bestPoint_in, mask, cv::RANSAC);
        /*=============================================================*/
        if(use_project)    OutputProjectionTrans(img_tmp, img_in, h, use_color);
        /*=============================================================*/
        if(use_rect){
            std::vector<cv::Point2f> dst_corners = CornerTrans(cv::Size(img_tmp.cols, img_tmp.rows), h);
            OutputRect(dst_corners, cv::Size(img_tmp.cols, img_in.rows), img_in, use_color);
        }
        /*=============================================================*/
        if(use_inlier)    OutputInlierMatch(img_tmp, keyImg_tmp, img_in, keyImg_in, bestMatches, mask);
    }
    /*=================================================================*/
    if(use_all)    OutputAllMatch(img_tmp, keyImg_tmp, img_in, keyImg_in, bestMatches);
    /*=================================================================*/

    cv::waitKey();
    return 0;
}


static void help(){
    std::cout << "\n This program demonstrates how to detect compute and match ORB BRISK and AKAZE descriptors \n"
        "Usage: \n"
        "  ./matchmethod_orb_akaze_brisk --image1=<image1(../data/basketball1.png as default)> --image2=<image2(../data/basketball2.png as default)>\n"
        "Press a key when image window is active to change algorithm or descriptor\n";
}

void CornerSort( std::vector<cv::Point2f> &corners, cv::Size img_in_size){

    int i;
    int max_x, max_y, min_x, min_y;
    max_x = min_x = corners[0].x ;
    max_y = min_y = corners[0].y ;
    for(i = 1;i < 4;i++){
	if( max_x < corners[i].x) max_x = corners[i].x;
	if( max_y < corners[i].y) max_y = corners[i].y;
	if( min_x > corners[i].x) min_x = corners[i].x;
	if( min_y > corners[i].y) min_y = corners[i].y;
    }

    corners[0] = cv::Point2f(min_x, min_y);
    corners[1] = cv::Point2f(max_x, min_y);
    corners[2] = cv::Point2f(max_x, max_y);
    corners[3] = cv::Point2f(min_x, max_y);

    for(int i = 0; i < 4; i++){
        if(corners[i].x < 0) corners[i].x = 0 ;
        if(corners[i].y < 0) corners[i].y = 0 ;
        if(corners[i].x > img_in_size.width)  corners[i].x = img_in_size.width-1 ;
        if(corners[i].y > img_in_size.height) corners[i].y = img_in_size.height-1 ;
    }
    return;
}

bool JudgeSuccess(const std::pair<int, int> &pred, const std::pair<int, int> &GT, const int eps){
    bool judge;
    int x_eps, y_eps;
    x_eps = std::abs(pred.first - GT.first);
    y_eps = std::abs(pred.second - GT.second);
    if ((x_eps <= eps) && (y_eps <= eps))   judge = true;
    else    judge = false;
    return judge;
}

std::vector<cv::Point2f> CornerTrans(cv::Size img_tmp_size, cv::Mat h){
    std::vector<cv::Point2f> src_corners(4);
    std::vector<cv::Point2f> dst_corners(4);
    src_corners[0] = cv::Point2f(.0f, .0f);// 左上
    src_corners[1] = cv::Point2f(static_cast<float>(img_tmp_size.width), .0f);// 右上
    src_corners[2] = cv::Point2f(static_cast<float>(img_tmp_size.width), static_cast<float>(img_tmp_size.height));// 右下
    src_corners[3] = cv::Point2f(.0f, static_cast<float>(static_cast<float>(img_tmp_size.height)));// 左下

    for(int i = 0; i < 4; i++){
        dst_corners[i].x = (h.at<double>(0,0)*src_corners[i].x)+(h.at<double>(0,1)*src_corners[i].y)+h.at<double>(0,2);
        dst_corners[i].x /= (h.at<double>(2,0)*src_corners[i].x)+(h.at<double>(2,1)*src_corners[i].y)+h.at<double>(2,2);
        dst_corners[i].y = (h.at<double>(1,0)*src_corners[i].x)+(h.at<double>(1,1)*src_corners[i].y)+h.at<double>(1,2);
        dst_corners[i].y /= (h.at<double>(2,0)*src_corners[i].x)+(h.at<double>(2,1)*src_corners[i].y)+h.at<double>(2,2);
    }
    return dst_corners;
}

void OutputProjectionTrans(const cv::Mat img_tmp, const cv::Mat img_in, cv::Mat h, bool use_color){
    cv::Mat tmp = img_tmp.clone();
    cv::Mat result = img_in.clone();
    if(use_color)    cv::rectangle(tmp, cv::Rect(0, 0, tmp.cols, tmp.rows), cv::Scalar(255, 255, 0), 4);
    else    cv::rectangle(tmp, cv::Rect(0, 0, tmp.cols, tmp.rows), cv::Scalar(128), 4);
    cv::warpPerspective(tmp, result, h, img_in.size(), 1, cv::BORDER_TRANSPARENT);
    cv::imshow("result_warp", result);
    return;
}

void OutputRect(std::vector<cv::Point2f> dst_corners, cv::Size img_tmp_size, cv::Mat img_in, bool use_color){
    CornerSort(dst_corners, cv::Size(img_in.cols, img_in.rows));
    cv::Mat result_match_line = DrawLines(img_in, dst_corners, use_color);
    cv::imshow("result_rect", result_match_line);
    return;
}

void OutputAllMatch(cv::Mat img_tmp, std::vector<cv::KeyPoint> keyImg_tmp, cv::Mat img_in, std::vector<cv::KeyPoint> keyImg_in, std::vector<cv::DMatch> bestMatches){
    cv::Mat result;
    cv::drawMatches(img_tmp, keyImg_tmp, img_in, keyImg_in, bestMatches, result); // 引数の順番注意
    cv::imshow("matching_pair", result);
    return;
}

void OutputInlierMatch(cv::Mat img_tmp, std::vector<cv::KeyPoint> keyImg_tmp, cv::Mat img_in, std::vector<cv::KeyPoint> keyImg_in, std::vector<cv::DMatch> bestMatches, cv::Mat mask){
    std::vector<cv::DMatch> inlierMatches;
    for (int i = 0; i < mask.rows; ++i) {
        uchar *inlier = mask.ptr<uchar>(i);
        if (inlier[0] == 1)    inlierMatches.push_back(bestMatches[i]);
    }
    cv::Mat result;
    cv::drawMatches(img_tmp, keyImg_tmp, img_in, keyImg_in, inlierMatches, result); // 引数の順番注意
    cv::imshow("matching_inlier", result);
    return;
}
