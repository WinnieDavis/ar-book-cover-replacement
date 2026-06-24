/**
* Winnie Davis - 2026
* 
*/

#include "Timer.h"
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>

void printUsage() {
	std::cout << "USAGE:" << std::endl;
	std::cout << "objectReplacement <video file> <target image> <replacement image>" << std::endl;
	std::cout << "  [match ratio] [RANSAC threhsold] [min inliers]" << std::endl << std::endl;
	std::cout << "  <video file> video in which to replace target with replacement" << std::endl;
	std::cout << "  <target image> an image to look for in the video" << std::endl;
	std::cout << "  <repalcement image> the image to overwrite the target with" << std::endl;
	std::cout << "  [match ratio] (optional, default 0.8) threshold to reject ambiguous matches" << std::endl;
	std::cout << "  [RANSAC threshold] (optional, default 3.0) threshold (in pixels) to accept a feature match in RANSAC" << std::endl;
	std::cout << "  [min inliers] (optional, default 4) minimum number of inlier features to keep matches" << std::endl << std::endl;
	std::cout << "Optional parameters are read in order, so if you just want to change [min inliers] you will need" << std::endl;
	std::cout << "to provide default values for [match ratio] and [RANSAC threshold] so if want a minimum of 20 inliers:" << std::endl;
	std::cout << "  objectReplacement input.mp4 target.png replacement.png 0.8 3.0 20" << std::endl;
}





int main(int argc, char* argv[]) {

	// Check command line parameters (8 bc of test script I ran)
	if (argc < 4 || argc > 8) {
		std::cout << argc << std::endl;
		printUsage();

		for (int i = 0; i < argc; ++i) {
			std::cout << i << ": " << argv[i] << std::endl;
		}
		exit(-1);
	}

	// The video input file
	cv::VideoCapture video(argv[1]);

	// Default path vs testscript path
	std::string outputPath = (argc == 8) ? argv[7] : "output.mp4"; // x? 1:2 - if x do 1 else 2

	// Needed to save the files for later viewing
	double fps = video.get(cv::CAP_PROP_FPS); // get framerate
	cv::Size frameSize(video.get(cv::CAP_PROP_FRAME_WIDTH), (video.get(cv::CAP_PROP_FRAME_HEIGHT)));
	cv::VideoWriter writer(outputPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, frameSize);


	if (!video.isOpened()) {
		std::cerr << "Could not open video file " << argv[1] << std::endl;
		exit(-2);
	}

	// Target image to look for in each frame
	cv::Mat target = cv::imread(argv[2], cv::IMREAD_COLOR);
	if (target.empty()) {
		std::cerr << "Could not read target image from " << argv[2] << std::endl;
		exit(-3);
	}

	// Replacement image to write over each frame where the target is found
	cv::Mat replacement = cv::imread(argv[3], cv::IMREAD_COLOR);
	if (replacement.empty()) {
		std::cerr << "Could not read target image from " << argv[2] << std::endl;
		exit(-4);
	}

	// The two images should be the same size, if not it might get weird...
	if (target.size() != replacement.size()) {
		std::cerr << "Target and replacement images are different sizes, this might get weird" << std::endl;
		std::cerr << "Target size: " << target.size() << " != Replacement size: " << replacement.size() << std::endl;
	}

	// Paramters to experiment with initialised to default unless provided

	// Ratio threshold used to disambiguate feature matches
	double ratioThreshold = 0.8;
	if (argc >= 5) {
		ratioThreshold = std::stod(argv[4]);
		if (ratioThreshold <= 0 || ratioThreshold > 1) {
			std::cerr << "Invalid ratio threshold '" << argv[5] << "'" << std::endl;
			std::cerr << "Ratio threshold should be in the range (0,1]" << std::endl;
		}

	}

	// Threshold in pixels for matches to be considered an inlier in homography estimation
	double ransacThreshold = 3.0;
	if (argc >= 6) {
		ransacThreshold = std::stod(argv[5]);
		if (ransacThreshold <= 0) {
			std::cerr << "Invalid RANSAC threshold '" << argv[5] << "'" << std::endl;
			std::cerr << "RANSAC threshold should be strictly positive" << std::endl;
		}
	}

	// Minimum number of inlier matches to accept that the target has been found
	// Note that at least four are required to estimate a homography
	int minInliers = 4;
	if (argc >= 7) { 
		minInliers = std::stoi(argv[6]);
		if (minInliers < 4) {
			std::cerr << "Invalid inlier threshold '" << argv[6] << "'" << std::endl;
			std::cerr << "Minimum number of inliers should be at least 4" << std::endl;
		}
	}

	cv::namedWindow("Display", cv::WINDOW_NORMAL); 
	cv::resizeWindow("Display", 1280, 720); 
	int noKey = cv::waitKey(10); 


	 // Initialise Timer frame and framecount
	Timer frameTimer;
	cv::Mat frame;
	size_t frameCount = 0;

	// Create a SIFT detector and FLANN matcher
	cv::Ptr<cv::Feature2D>detector = cv::SIFT::create();
	cv::Ptr<cv::DescriptorMatcher>matcher = cv::FlannBasedMatcher::create();

	// Detect features in the target image. noArray() means no mask to restict feature detection to 
	std::vector<cv::KeyPoint> keypointsTarget;
	cv::Mat descriptorsTarget;

	detector->detectAndCompute(target, cv::noArray(), keypointsTarget, descriptorsTarget);
	matcher->add(descriptorsTarget);
	matcher->train();


	// Loop until a key is pressed (or break on end of video)
	while (cv::waitKey(10) == noKey) { //ADDTHIS LINE BACKKKK
		

			// Read next frame - outside of timing loop
			video >> frame;


			if (frame.empty()) {
				// End of video, stop the loop
				break;
			}
			++frameCount;

			// Main timing loop - may want internal timers but avoid non-computational stuff in here
			// In particular I/O can disrupt timing as it can lead to processes blocking
			frameTimer.reset();

			 // Detect feautres in this frame 
			std::vector<cv::KeyPoint> keypointsVid;
			cv::Mat des, descriptorsVid;
			detector->detectAndCompute(frame, cv::noArray(), keypointsVid, descriptorsVid);

			// Match them to the target image
			std::vector<std::vector<cv::DMatch>> matches;
			matcher->knnMatch(descriptorsVid, matches, 2);

			// Filter matches with ratio test 
			std::vector<cv::DMatch> goodMatches;
			std::vector<cv::Point2f> goodPts1, goodPts2;

			for (const auto& match : matches) {
				if (match[0].distance < ratioThreshold * match[1].distance) {
					goodMatches.push_back(match[0]);
					// queryIdx = index into keypoints1 (first image)
					// trainIdx = index into keypoints2 (second image)
					goodPts1.push_back(keypointsVid[match[0].queryIdx].pt);
					goodPts2.push_back(keypointsTarget[match[0].trainIdx].pt);
				}
			}

			// Find Homography with RANSAC   
			std::vector<unsigned char> inliers;
			cv::Mat H = cv::findHomography(goodPts2, goodPts1, inliers, cv::RANSAC, ransacThreshold);


			// If there are sufficient inliers warp
			int inlierCount = 0;
			for (auto val : inliers) if (val) inlierCount++; //if not 0 then true


			cv::Mat screenSizeCanvas(frame.size(), CV_8UC3);
			cv::Mat mask(replacement.size(), CV_8UC1, cv::Scalar(255)); //white mask
			cv::Mat warpedMask(frame.size(), CV_8UC1);

			if (inlierCount > minInliers) {
				// warp the replacement image to the target image
				cv::warpPerspective(replacement, screenSizeCanvas, H, frame.size());

				// mask is warped the same with h for region
				cv::warpPerspective(mask, warpedMask, H, frame.size());

				// put on frame
				cv::Mat output = frame.clone();
				screenSizeCanvas.copyTo(output, warpedMask);

				cv::imshow("Display", output);
			}

			// Read the timer
			double frameTime = frameTimer.elapsed();

			//Code to add results to test file and save video 
			cv::Mat outputFrame = frame.clone();
			if (inlierCount > minInliers) {
				screenSizeCanvas.copyTo(outputFrame, warpedMask);
			}
			writer.write(outputFrame);


			std::cout << "CSV," << frameCount << "," << frameTime << ","
				<< goodMatches.size() << "," << inlierCount << ","
				<< (inlierCount > minInliers ? 1 : 0) << std::endl;

			//Display 
			cv::imshow("Display", outputFrame);

	}
	return 0;
}
