/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_download_request_manager.h"

#include "base/ranges/algorithm.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/base/schemeful_site.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

class PlaylistDownloadRequestManagerBrowserTest : public PlatformBrowserTest {
 public:
  struct ExpectedData {
    std::string name;
    std::string thumbnail_source;
    std::string media_source;
    std::string duration;
  };

  PlaylistDownloadRequestManagerBrowserTest() {
    playlist::PlaylistDownloadRequestManager::SetPlaylistJavaScriptWorldId(
        ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }
  ~PlaylistDownloadRequestManagerBrowserTest() override = default;

  auto* https_server() { return https_server_.get(); }
  auto* request_manager() { return request_manager_.get(); }

  playlist::mojom::PlaylistItemPtr CreateItem(const ExpectedData& data) {
    auto item = playlist::mojom::PlaylistItem::New();
    item->name = data.name;
    item->thumbnail_source = GURL(data.thumbnail_source);
    item->thumbnail_path = GURL(data.thumbnail_source);
    item->media_source = GURL(data.media_source);
    item->media_path = GURL(data.media_source);
    item->duration = data.duration;
    return item;
  }

  void LoadHTMLAndCheckResult(const std::string& html,
                              const std::vector<ExpectedData>& items,
                              const GURL& url = GURL()) {
    const auto* test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    VLOG(2) << test_info->name() << ": " << __func__;

    if (https_server()->Started())
      ASSERT_TRUE(https_server()->ShutdownAndWaitUntilComplete());
    https_server()->RegisterRequestHandler(
        base::BindRepeating(&PlaylistDownloadRequestManagerBrowserTest::Serve,
                            base::Unretained(this), html));
    ASSERT_TRUE(https_server()->Start());

    // Load given |html| contents.
    auto* active_web_contents = chrome_test_utils::GetActiveWebContents(this);

    GURL destination_url = url;
    if (destination_url.is_valid()) {
      destination_url = https_server()->GetURL(destination_url.host(),
                                               destination_url.path());
    } else {
      destination_url = https_server()->GetURL("/test");
    }
    ASSERT_TRUE(content::NavigateToURL(active_web_contents, destination_url));

    // Run script and find media files
    ASSERT_FALSE(component_manager_->GetMediaDetectorScript({}).empty());
    playlist::PlaylistDownloadRequestManager::Request request;
    request.url_or_contents = active_web_contents->GetWeakPtr();
    request.callback =
        base::BindOnce(&PlaylistDownloadRequestManagerBrowserTest::OnGetMedia,
                       base::Unretained(this), test_info->name(), items,
                       url.is_valid() ? url.host() : destination_url.host());
    request_manager_->GetMediaFilesFromPage(std::move(request));

    // Block until result is received from OnGetMedia().
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

 protected:
  // PlatformBrowserTest:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);

    component_manager_ =
        std::make_unique<playlist::MediaDetectorComponentManager>(nullptr);
    component_manager_->SetUseLocalScriptForTesting();

    request_manager_ =
        std::make_unique<playlist::PlaylistDownloadRequestManager>(
            chrome_test_utils::GetProfile(this), component_manager_.get());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void TearDownOnMainThread() override {
    request_manager_.reset();
    component_manager_.reset();

    if (https_server()->Started())
      ASSERT_TRUE(https_server()->ShutdownAndWaitUntilComplete());

    PlatformBrowserTest::TearDownOnMainThread();
  }

 private:
  std::unique_ptr<net::test_server::HttpResponse> Serve(
      const std::string& html,
      const net::test_server::HttpRequest& request) {
    GURL absolute_url = https_server()->GetURL(request.relative_url);
    auto response = std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_code(net::HTTP_OK);
    response->set_content(html);
    response->set_content_type("text/html; charset=utf-8");
    return response;
  }

  void OnGetMedia(const char* test_name,
                  std::vector<ExpectedData> expected_data,
                  const std::string& requested_host,
                  std::vector<playlist::mojom::PlaylistItemPtr> actual_items) {
    VLOG(2) << test_name << ": " << __func__;

    std::vector<playlist::mojom::PlaylistItemPtr> expected_items;
    base::ranges::for_each(expected_data, [&](ExpectedData& item) {
      auto fix_host = [&](auto& url_str) {
        if (!base::StartsWith(url_str, "/")) {
          return;
        }

        // Fix up host so that we can drop port nums.
        GURL new_url = https_server()->GetURL(url_str);
        EXPECT_TRUE(new_url.is_valid());
        GURL::Replacements replacements;
        replacements.SetHostStr(requested_host);
        url_str = new_url.ReplaceComponents(replacements).spec();
      };

      fix_host(item.thumbnail_source);
      fix_host(item.media_source);

      expected_items.push_back(CreateItem(item));
    });

    EXPECT_EQ(actual_items.size(), expected_items.size());

    auto equal = [](const auto& a, const auto& b) {
      // id is not compared because it is generated for actual items.
      return a->media_path == b->media_path && a->name == b->name &&
             a->thumbnail_path == b->thumbnail_path;
    };
    EXPECT_TRUE(base::ranges::equal(actual_items, expected_items, equal));

    ASSERT_TRUE(run_loop_);
    run_loop_->Quit();
  }

  std::unique_ptr<playlist::MediaDetectorComponentManager> component_manager_;
  std::unique_ptr<playlist::PlaylistDownloadRequestManager> request_manager_;

  std::unique_ptr<base::RunLoop> run_loop_;

  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest, NoMedia) {
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
        </body></html>
      )html",
      {});
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       SrcAttributeTest) {
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
          <video src="test.mp4"/>
        </body></html>
      )html",
      {{.name = "",
        .thumbnail_source = "",
        .media_source = "/test.mp4",
        .duration = ""}});
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       SrcElementTest) {
  LoadHTMLAndCheckResult(
      R"html(
        <html><body>
          <video>
            <source src="test1.mp4"/>
            <source src="test2.mp4"/>
          </video>
        </body></html>
      )html",
      {{.name = "",
        .thumbnail_source = "",
        .media_source = "/test1.mp4",
        .duration = ""},
       {.name = "",
        .thumbnail_source = "",
        .media_source = "/test2.mp4",
        .duration = ""}});
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       YouTubeSpecificRetriever) {
  // Pre-conditions to decide site specific script
  ASSERT_EQ(net::SchemefulSite(GURL("https://m.youtube.com")),
            net::SchemefulSite(GURL("https://youtube.com")));
  ASSERT_EQ(net::SchemefulSite(GURL("https://youtube.com")),
            net::SchemefulSite(GURL("https://www.youtube.com")));
  ASSERT_NE(net::SchemefulSite(GURL("http://m.youtube.com")),
            net::SchemefulSite(GURL("https://m.youtube.com")));

  // Getting JavaScript object requires to access the main world.
  request_manager()->SetRunScriptOnMainWorldForTest();

  // Check if we can retrieve metadata from youtube specific script.
  LoadHTMLAndCheckResult(
      R"html(
        <html>
        <script>
          window.ytplayer = {
            "bootstrapPlayerResponse": {
              "videoDetails": {
                "videoId": "12345689",
                "title": "Dummy response",
                "lengthSeconds": "200.123",
                "keywords": [
                  "keyword"
                ],
                "channelId": "channel-id",
                "isOwnerViewing": false,
                "shortDescription": "this is dummy data for youtube object",
                "isCrawlable": true,
                "thumbnail": {
                  "thumbnails": [
                    {
                      "url": "/thumbnail.jpg",
                      "width": 1920,
                      "height": 1080
                    }
                  ]
                },
                "allowRatings": true,
                "viewCount": "1",
                "author": "Me",
                "isPrivate": false,
                "isUnpluggedCorpus": false,
                "isLiveContent": false
              }
            }
          };
        </script>
        <body>
          <video src="test.mp4"></video>
        </body></html>
      )html",
      {
          {.name = "Dummy response",
           .thumbnail_source = "/thumbnail.jpg",
           .media_source = "/test.mp4",
           .duration = "200.123"},
      },
      GURL("https://m.youtube.com/"));

  ASSERT_EQ(net::SchemefulSite(GURL("https://m.youtube.com")),
            net::SchemefulSite(https_server()->GetURL("m.youtube.com", "/")));
}

class PlaylistDownloadRequestManagerWithFakeUABrowserTest
    : public PlaylistDownloadRequestManagerBrowserTest {
 public:
  PlaylistDownloadRequestManagerWithFakeUABrowserTest()
      : scoped_feature_list_(playlist::features::kPlaylistFakeUA) {}
  ~PlaylistDownloadRequestManagerWithFakeUABrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerWithFakeUABrowserTest,
                       FakeUAStringContainsIPhone) {
  const auto user_agent_string = request_manager()
                                     ->GetBackgroundWebContentsForTesting()
                                     ->GetUserAgentOverride()
                                     .ua_string_override;

  EXPECT_TRUE(base::Contains(user_agent_string, "iPhone"));
  EXPECT_FALSE(base::Contains(user_agent_string, "Chrome"));
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       OGTagImageWithAbsolutePath) {
  LoadHTMLAndCheckResult(
      R"html(
        <html>
        <meta property="og:image" content="https://foo.com/img.jpg">
        <body>
          <video>
            <source src="test1.mp4"/>
          </video>
        </body></html>
      )html",
      {
          {.name = "",
           .thumbnail_source = "https://foo.com/img.jpg",
           .media_source = "/test1.mp4",
           .duration = ""},
      });
}

IN_PROC_BROWSER_TEST_F(PlaylistDownloadRequestManagerBrowserTest,
                       OGTagImageWithRelativePath) {
  LoadHTMLAndCheckResult(
      R"html(
        <html>
        <meta property="og:image" content="/img.jpg">
        <body>
          <video>
            <source src="test1.mp4"/>
          </video>
        </body></html>
      )html",
      {
          {.name = "",
           .thumbnail_source = "/img.jpg",
           .media_source = "/test1.mp4",
           .duration = ""},
      });
}
