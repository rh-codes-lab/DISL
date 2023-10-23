#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_event.h>
#include <esp_ota_ops.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <esp_http_client.h>
#include <esp_sntp.h>
#include <esp_random.h>
#include <mdns.h>
#include <nvs_flash.h>
#include <mqtt_client.h>
#include "appdefs.h"
#include "appmqtt.h"
#include "appwebserver.h"
#include "appwifi.h"
#include "appstate.h"
#include "esp_netif.h"
#include "ssd1306.h"

#define REGISTER_WIFI_SSID getHostname()

extern char ssid[sizeof(((wifi_sta_config_t*) NULL)->ssid)];
extern char pwd[sizeof(((wifi_sta_config_t*) NULL)->password)];

static esp_netif_t* ap;
ssid_info_t** aps;
static char ap_password[13];
static char* TAG = "appwebserver";

static void decode(char* buf)
{
  const char* rp = buf;
  char* wp = buf;
  while (*rp != '\0')
    switch (*rp) {
    case '+':
      *wp++ = ' ';
      ++rp;
      break;
    case '%':
      assert(*rp != '%');
      break;
    default:
      *wp++ = *rp++;
      break;
    }

  *wp = '\0';
}


static esp_err_t serve_connect_post_handler(httpd_req_t* req)
{
  int orig = req->content_len;
  int rem = orig;
  ESP_LOGI(TAG, "POST len=%d", rem);
  if (rem <= 10)
    return ESP_FAIL;

  char* buf = malloc(rem + 1);
  assert(buf != NULL);
  char* cp = buf;
  do {
    ssize_t n = httpd_req_recv(req, cp, rem);
    if (n <= 0) 
    {
      free(buf);
      return ESP_FAIL;
    }
    cp += n;
    rem -= n;
  } while (rem > 0);

  *cp = '\0';
  ESP_LOGI(TAG, "POST '%s'", buf);

  size_t copylen = orig - 9;
  char* reqssid = malloc(copylen);
  char* reqpwd = malloc(copylen);
  assert(reqssid != NULL && reqpwd != NULL);

  esp_err_t res = ESP_FAIL;
  if (httpd_query_key_value(buf, "ssid", reqssid, copylen) == ESP_OK &&
      httpd_query_key_value(buf, "pwd", reqpwd, copylen) == ESP_OK) 
  {
    decode(reqssid);
    decode(reqpwd);

    // Stop AP mode.
    esp_netif_dhcps_stop(ap);
    ESP_ERROR_CHECK(esp_wifi_stop());

    strncpy(ssid, reqssid, sizeof(ssid));
    ssid[sizeof(ssid) - 1] = '\0';
    strncpy(pwd, reqpwd, sizeof(pwd));
    pwd[sizeof(pwd) - 1] = '\0';

    xEventGroupSetBits(*(getWifiEventGroup()), HAS_ADDRESS_BIT);

    res = ESP_OK;
  }

  free(reqpwd);
  free(reqssid);
  free(buf);
  return res;
}


static esp_err_t serve_root_get_handler(httpd_req_t *req)
{
  httpd_resp_sendstr_chunk(req, R"**(<!DOCTYPE html><html> <head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <style type="text/css">
.all {
  background: darkblue;
  color: yellow;
  font-family: sans-serif;
}
.t {
  font-size: 4vw;
}
span {
  margin: 3vw;
}
td:nth-of-type(1) {
  text-align: right;
}
th:nth-of-type(2) {
  text-align: left;
}
td:nth-of-type(2) {
  border: 5px solid gold;
}
td:nth-of-type(2):hover {
  background: orange;
  color: black;
}
button, input[type=text] {
  font-size: 2vw;
}
    </style>
  </head>
  <body class="all">
    <dialog id="dialog" class="all t">
      <form method="post" id="form" action="connect.html">
        <label for="pwd">Password: </label><br>
        <input id="pwd" type="text" name="pwd">
        <input id="ssid" type="hidden" name="ssid">
      </form>
      <div style="text-align: center;">
        <button onclick="submit()">Submit</button>
        <button onclick="document.getElementById('dialog').close()">Cancel</button>
      </div>
    </dialog>
    <table class="t" style="width: 100%; border-spacing: 3vh;">
      <thead>
        <tr>
          <th></th>
          <th><span>SSID</span></th>
        </tr>
      </thead>
      <tbody>)**");

  for (unsigned i = 0; i < getAPCount(); ++i) {
    httpd_resp_sendstr_chunk(req, "<tr><td>");
    httpd_resp_sendstr_chunk(req, aps[i]->auth ? u8"\U0001f512</td><td><span onclick='ask(\"" : u8"\U0001f513</td><td><span onclick='ask(\"");
    httpd_resp_sendstr_chunk(req, aps[i]->ssid);
    httpd_resp_sendstr_chunk(req, "\",");
    httpd_resp_sendstr_chunk(req, aps[i]->auth ? "true" : "false");
    httpd_resp_sendstr_chunk(req, ")'>");
    httpd_resp_sendstr_chunk(req, aps[i]->ssid);
    httpd_resp_sendstr_chunk(req, "</td></tr>");
  }

  httpd_resp_sendstr_chunk(req, R"**(</tbody>
    </table>
    <script type="text/javascript">
var dialog = document.getElementById('dialog'), form = document.getElementById("form"), ssid = document.getElementById("ssid");
function ask(name,needpwd) {
  form.reset();
  ssid.value = name;
  if (needpwd)
    dialog.showModal();
  else
    form.submit();
}
function submit() {
  var ssid = document.getElementById('ssid').value;
  document.getElementById('dialog').close();
  document.getElementById('form').submit();
  document.body.innerHTML = '<h1>Your ESP32 is now connecting to ' + ssid + '</h1>';
}
      </script>
  </body>
</html>)**");

  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t serve_root = {
  .uri = "/",
  .method = HTTP_GET,
  .handler = serve_root_get_handler,
  .user_ctx = NULL
};


static const httpd_uri_t serve_connect = {
  .uri = "/connect.html",
  .method = HTTP_POST,
  .handler = serve_connect_post_handler,
  .user_ctx = NULL
};

httpd_handle_t start_webserver(bool ap_mode)
{
  esp_err_t res;
  ESP_LOGI(TAG, "Starting webserver %sin AP Mode",(ap_mode==1?"":"not "));
#if USE_SSL
  ESP_LOGI(TAG, "Using SSL");
  httpd_handle_t server = NULL;
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

  extern const unsigned char servercert_start[] asm("_binary_caroot_pem_start");
  extern const unsigned char servercert_end[]   asm("_binary_caroot_pem_end");
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
  config.servercert = servercert_start;
  config.servercert_len = servercert_end - servercert_start;
#else
  ESP_LOGI(TAG, "Using ESP-IDF-v4.4.4");
  config.cacert_pem = servercert_start;
  config.cacert_len = servercert_end - servercert_start;
#endif

  extern const unsigned char prvtkey_pem_start[] asm("_binary_cakey_pem_start");
  extern const unsigned char prvtkey_pem_end[]   asm("_binary_cakey_pem_end");
  config.prvtkey_pem = prvtkey_pem_start;
  config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

  ESP_LOGI(TAG, "Starting SSL server on port: '%d'", config.httpd.server_port);
  res = httpd_ssl_start(&server, &config);
#else
  ESP_LOGI(TAG, "Not using SSL");
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  res = httpd_start(&server, &config);
#endif
  if (res == ESP_OK) 
  {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    if(ap_mode == true)
    {
      httpd_register_uri_handler(server, &serve_root);
      httpd_register_uri_handler(server, &serve_connect);
    }
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}


void stop_webserver(httpd_handle_t handle)
{
#if USE_SSL
  httpd_ssl_stop(handle);
#else
  httpd_stop(handle);
#endif
}

static void genpwd(char* pwd, size_t len)
{
  typedef uint32_t rand_type;
#define MAXRAND 0xffffffff
#define RAND() esp_random()
#define NCHARS 83
  static const char all[NCHARS] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*(){}[]:<>?,./";
  static const char cls[NCHARS] = "00000000000000000000000000111111111111111111111111112222222222333333333333333333333";

  while (1) 
  {
    bool has_cls[4] = { false, false, false, false };

    for (size_t l = 0; l < len; ++l) 
    {
      rand_type r;
      do
        r = RAND();
      while (r >= MAXRAND - (MAXRAND % NCHARS));
      r %= NCHARS;

      pwd[l] = all[r];
      has_cls[cls[r] & 0x3] = true;
    }

    if (has_cls[0] & has_cls[1] & has_cls[2] & has_cls[3])
    {
      return;
    }
  }
}


void create_ap(void)
{
  if (ap == NULL)
  {
    ap = esp_netif_create_default_wifi_ap();
  }
  assert(ap != NULL);

  esp_netif_ip_info_t ipinfo;
  IP4_ADDR(&ipinfo.ip, 192,168,4,1);
  IP4_ADDR(&ipinfo.gw, 192,168,4,1);
  IP4_ADDR(&ipinfo.netmask, 255,255,255,0);
  esp_netif_set_ip_info(ap, &ipinfo);
  esp_netif_dhcps_start(ap);

  genpwd(ap_password, sizeof(ap_password) - 1);
  ap_password[sizeof(ap_password) - 1] = '\0';

  wifi_config_t wifi_config = {
    .ap = {
      .channel = REGISTER_WIFI_CHANNEL,
      .max_connection = MAX_STA_CONN,
      .authmode = WIFI_AUTH_WPA_WPA2_PSK
    },
  };
  strncpy((char*) wifi_config.ap.ssid, REGISTER_WIFI_SSID, sizeof(wifi_config.ap.ssid));
  wifi_config.ap.ssid_len = strlen(REGISTER_WIFI_SSID);
  strncpy((char*) wifi_config.ap.password, REGISTER_WIFI_PASS, sizeof(wifi_config.ap.password));
  if (strlen(REGISTER_WIFI_PASS) == 0)
  {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));

  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi AP setup finished. SSID:%s password:%s channel:%d", REGISTER_WIFI_SSID, REGISTER_WIFI_PASS, REGISTER_WIFI_CHANNEL);

// EldritchJS Add this for monochrome OLED
#if CONFIG_OLED_ENABLE
char dstr[256];
ssd1306DisplayClear();
sprintf(dstr, "SSID:%s\nPASS:%s", REGISTER_WIFI_SSID, REGISTER_WIFI_PASS);
ssd1306_display_text(dstr);
#endif

  EventGroupHandle_t* wifi_event_group = getWifiEventGroup();
  *wifi_event_group = xEventGroupCreate();

  httpd_handle_t handle = start_webserver(true);
  assert(handle != NULL);

  EventBits_t bits;
  do
    bits = xEventGroupWaitBits(*wifi_event_group, HAS_ADDRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  while ((bits & HAS_ADDRESS_BIT) == 0);

  stop_webserver(handle);

  vEventGroupDelete(*wifi_event_group);
}



