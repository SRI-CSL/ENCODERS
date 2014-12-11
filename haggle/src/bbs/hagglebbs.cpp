/* Copyright 2008 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *     
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */ 

#include <list>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include <libhaggle/haggle.h>

#include "httpd.h"
#include "databuf.h"

#include "mini_base64.h"

class bbs_posting {
	static long	id_counter;
	long		id;
	char		*text;
	char		*author;
	char		*date_of_post;
public:
	char *get_text(void);
	char *get_author(void);
	char *get_date_of_post(void);
	long get_id(void);
	
	bbs_posting(
		const char *text, 
		const char *author, 
		const char *date_of_post);
	~bbs_posting();
};

class bbs_topic {
	static long					id_counter;
	long						id;
	char						*topic;
	std::list<bbs_posting *>	posts;
	
public:
	std::list<bbs_posting *> get_posts(void);
	void add_post(bbs_posting *post);
	char *get_topic(void);
	long get_id(void);
	
	bbs_topic(const char *topic);
	~bbs_topic();
};

long	bbs_posting::id_counter = 0;
long	bbs_topic::id_counter = 0;
	
static char *haggle_bbs_attribute_name = (char *) "HaggleBBS";
static char *haggle_bbs_attribute_post_value = (char *) "Post";
static char *haggle_bbs_topic_attribute_name = (char *) "HaggleBBS-Topic";
static char *haggle_bbs_text_attribute_name = (char *) "HaggleBBS-Post";
static char *haggle_bbs_author_attribute_name = (char *) "HaggleBBS-Author";
static char *haggle_bbs_date_attribute_name = (char *) "HaggleBBS-Date";
static std::list<bbs_topic *> topics;

static haggle_handle_t haggle_;

int onDataObject(haggle_event_t *e, void *arg);
static void signal_handler(int signal);
bool insert_chars(char **txt, long txt_len, long where, long how_many);
void board_text_fix(char **txt, long *txt_len, bool fix_newlines);
void un_url_encode(char *text, long max_len);
bbs_posting *new_post(struct dataobject *dObj);
bbs_topic *get_topic(const char *topic);
bbs_topic *get_topic_by_id(long which);
void add_post(char *topic, char *text);

data_buffer	get_new_topic_page(void);
data_buffer	get_new_post_page(bbs_topic *topic);

/*
	Returns the text of a posting
*/
char *bbs_posting::get_text(void)
{
	return text;
}

/*
	Returns the author of a posting
*/
char *bbs_posting::get_author(void)
{
	return author;
}

/*
	Returns the date a post was posted
*/
char *bbs_posting::get_date_of_post(void)
{
	return date_of_post;
}

/*
	Returns a post ID.
*/
long bbs_posting::get_id(void)
{
	return id;
}

/*
	Post constructor
*/
bbs_posting::bbs_posting(
		const char *_text, 
		const char *_author, 
		const char *_date_of_post) : 
	id(id_counter++)
{
	long	text_len;
	
	printf("New posting: \n"
		"\ttext: \"%s\" (%ld chars)\n"
		"\tauthor: \"%s\" (%ld chars)\n"
		"\tdate of post: \"%s\" (%ld chars)\n",
		_text, (long) strlen(_text), 
		_author, (long) strlen(_author),
		_date_of_post, (long) strlen(_date_of_post));
	
	text_len = strlen(_text) + 1;
	if(text_len == 0)
		throw 42;
	text = (char *) malloc(text_len);
	if(text == NULL)
		throw 42;
	memcpy(text, _text, text_len);
	board_text_fix(&text, &text_len, true);
	
	text_len = strlen(_author) + 1;
	if(text_len == 0)
		throw 42;
	author = (char *) malloc(text_len);
	if(author == NULL)
		throw 42;
	memcpy(author, _author, text_len);
	board_text_fix(&author, &text_len, true);
	
	text_len = strlen(_date_of_post) + 1;
	if(text_len == 0)
		throw 42;
	date_of_post = (char *) malloc(text_len);
	if(date_of_post == NULL)
		throw 42;
	memcpy(date_of_post, _date_of_post, text_len);
	board_text_fix(&date_of_post, &text_len, true);
}

/*
	Post destructor
*/
bbs_posting::~bbs_posting()
{
	free(text);
}

char *get_attr_b64(struct dataobject *dObj, char *name)
{
	char		*data, *value;
	attribute	*attr;
	data_buffer	buf;
	
	attr = 
		haggle_dataobject_get_attribute_by_name(
			dObj, 
			name);
	if(attr == NULL)
		return NULL;
	value = (char *) haggle_attribute_get_value(attr);
	if(value == NULL)
		return NULL;
	
	buf =
		mini_base64_decode(
			value,
			strlen(value));
	if(buf.data != NULL)
	{
		data = (char *) realloc(buf.data, buf.len + 1);
		if(data == NULL)
			return buf.data;
		
		data[buf.len] = '\0';
	}
	return buf.data;
}

/*
	Creates a new posting from the given data object. The posting is created,
	but not inserted anywhere or otherwise managed.
*/
bbs_posting *new_post(struct dataobject *dObj)
{
	char		*the_text, *the_author, *the_date;
	bbs_posting	*retval;
	
	retval = NULL;
	
	the_text = 
		get_attr_b64(
			dObj, 
			haggle_bbs_text_attribute_name);
	if(the_text == NULL)
		goto fail_text;
	the_author = 
		get_attr_b64(
			dObj, 
			haggle_bbs_author_attribute_name);
	if(the_author == NULL)
		goto fail_author;
	the_date = 
		get_attr_b64(
			dObj, 
			haggle_bbs_date_attribute_name);
	if(the_date == NULL)
		goto fail_date;
	
	try{
		return new 
			bbs_posting(
				the_text,
				the_author,
				the_date);
	}catch(int){
	};
	
	free(the_date);
fail_date:
	free(the_author);
fail_author:
	free(the_text);
fail_text:
	return retval;
}

/*
	Returns the topic description.
*/
char *bbs_topic::get_topic(void)
{
	return topic;
}

/*
	Returns the id of this topic.
*/
long bbs_topic::get_id(void)
{
	return id;
}

/*
	Returns the list of postings for the given topic.
*/
std::list<bbs_posting *> bbs_topic::get_posts(void)
{
	return posts;
}

/*
	Adds a post to a topic.
*/
void bbs_topic::add_post(bbs_posting *post)
{
	if(post == NULL)
		return;
	
	// Check for duplicate posts:
	for(std::list<bbs_posting *>::iterator it = posts.begin();
		it != posts.end();
		it++)
	{
		if(	strcmp(post->get_text(), (*it)->get_text()) == 0 &&
			strcmp(post->get_author(), (*it)->get_author()) == 0 &&
			strcmp(post->get_date_of_post(), (*it)->get_date_of_post()) == 0)
		{
			delete post;
			return;
		}
	}
	
	// Find where to insert
	for(std::list<bbs_posting *>::iterator it = posts.begin();
		it != posts.end();
		it++)
	{
		// Is the date lower (older) than this post's date?
		if(strcmp(post->get_date_of_post(), (*it)->get_date_of_post()) < 0)
		{
			// insert here
			posts.insert(it, post);
			return;
		}
	}
	// Insert last:
	posts.push_back(post);
}

/*
	Topic constructor
*/
bbs_topic::bbs_topic(const char *_topic) : id(id_counter++)
{
	long	topic_len;
	
	topic_len = strlen(_topic) + 1;
	if(topic_len == 0)
		throw 42;
	topic = (char *) malloc(topic_len);
	if(topic == NULL)
		throw 42;
	strcpy(topic, _topic);
	board_text_fix(&topic, &topic_len, true);
}

/*
	Topic destructor
*/
bbs_topic::~bbs_topic()
{
	while(posts.size() > 0)
	{
		delete posts.front();
		posts.pop_front();
	}
	free(topic);
}

/*
	Returns the topic with the given topic title. If none is found, one is 
	created (unless something goes terribly wrong).
*/
bbs_topic *get_topic(const char *topic)
{
	bbs_topic	*retval;
	
	for(std::list<bbs_topic *>::iterator it = topics.begin();
		it != topics.end();
		it++)
	{
		if(strcmp(topic, (*it)->get_topic()) == 0)
			return (*it);
	}
	
	try {
		retval = new bbs_topic(topic);
	}catch(int){
		retval = NULL;
	};
	
	if(retval != NULL)
		topics.push_back(retval);
	
	return retval;
}

/*
	Returns the topic with the given ID.
*/
bbs_topic *get_topic_by_id(long which)
{
	for(std::list<bbs_topic *>::iterator it = topics.begin();
		it != topics.end();
		it++)
	{
		if((*it)->get_id() == which)
			return (*it);
	}
	return NULL;
}

void add_attr_b64(struct dataobject *dObj, char *name, char *value)
{
	data_buffer buf;
	
	buf = mini_base64_encode(value, strlen(value)+1);
	haggle_dataobject_add_attribute(
		dObj,
		name,
		buf.data);
	free(buf.data);
}

/*
	Insert a post to the board. This actually only creates a correct data 
	object, and sends it to haggle, but haggle will in turn return the data 
	object since it will match the application interests set up in main().
*/
void add_post(char *topic, char *text, char *author)
{
	struct dataobject	*dObj;
	char		str[32];
	time_t		now;
	struct tm	now_tm;
	
	// Create posting time:
	now = time(NULL);
	(void) gmtime_r(&now, &now_tm);
	sprintf(
		str,
		"%04d-%02d-%02d %02d:%02d:%02d GMT", 
		1900 + now_tm.tm_year, 
		now_tm.tm_mon, 
		now_tm.tm_mday, 
		now_tm.tm_hour, 
		now_tm.tm_min, 
		now_tm.tm_sec);
	
	dObj = haggle_dataobject_new();
	
	// Make sure the data object is permanent:
	haggle_dataobject_set_flags(dObj, DATAOBJECT_FLAG_PERSISTENT);
	
	haggle_dataobject_add_attribute(
		dObj,
		haggle_bbs_attribute_name,
		haggle_bbs_attribute_post_value);
	
	add_attr_b64(
		dObj,
		haggle_bbs_topic_attribute_name,
		topic);
	
	add_attr_b64(
		dObj,
		haggle_bbs_text_attribute_name,
		text);
	
	add_attr_b64(
		dObj,
		haggle_bbs_author_attribute_name,
		author);
	
	add_attr_b64(
		dObj,
		haggle_bbs_date_attribute_name,
		str);
	
	haggle_ipc_publish_dataobject(haggle_, dObj);
	
	haggle_dataobject_free(dObj);
}

/*
	This is called when haggle tells us there's a new data object we might be 
	interested in.
*/
int onDataObject(haggle_event_t *e, void *arg)
{
	char				*the_topic;
	bbs_topic			*topic;
	
	the_topic = 
		get_attr_b64(
			e->dobj, 
			haggle_bbs_topic_attribute_name);
	if(the_topic == NULL)
		goto fail_topic;
	
	topic = get_topic(the_topic);
	topic->add_post(new_post(e->dobj));
	
	free(the_topic);
        return 1;
fail_topic:
	return -1;
}

// Start of all haggle board pages (includes a nice picture):
static char *board_start = 
	(char *) "<HTML><BODY><center><img src=\"HaggleLogoBlue400.png\"></center>";

// End of all haggle board pages:
static char *board_end = (char *) 
	"<center>Copyright &copy; 2008 Uppsala Universitet.</center></BODY></HTML>";

#define add_text(dbuf, txt)	\
	((dbuf) = \
		data_buffer_cons_free( \
			(dbuf), \
			data_buffer_create_copy( \
				(txt), \
				strlen(txt))))

data_buffer	get_new_topic_page(void)
{
	data_buffer	new_topic_page;
	
	new_topic_page.data = NULL;
	new_topic_page.len = 0;
	
	// The haggle logo and start of the board:
	add_text(new_topic_page, board_start);
	
	add_text(new_topic_page, (char *) 
		"<center>"
			"<table width=100% border=1><tr><td valign=top>"
				"<form method=POST action=\"/new_topic\">"
					"Name: <input type=text name=\"name\"><br>"
					"Topic title: <input type=text name=\"topic\"><br>"
					"Message: "
						"<br>"
							"<textarea name=\"text\" cols=80 rows=20>"
							"</textarea>"
						"<br>"
					"<button name=\"submit\" value=\"submit\" type=\"submit\">"
						"Add topic"
					"</button>"
				"</form>"
			"</table>"
		"</center>");
	
	// The end of the board:
	add_text(new_topic_page, board_end);
	
	return new_topic_page;
}

data_buffer	get_new_post_page(bbs_topic *topic)
{
	data_buffer	new_post_page;
	char		str[32];
	
	new_post_page.data = NULL;
	new_post_page.len = 0;
	
	// The haggle logo and start of the board:
	add_text(new_post_page, board_start);
	
	add_text(new_post_page, (char *) 
		"<center>"
			"<table width=100% border=1><tr><td valign=top>"
				"<form method=POST action=\"/add_reply_");
	sprintf(str, "%ld", topic->get_id());
	add_text(new_post_page, str);
	add_text(new_post_page, (char *) 
						"\">"
					"Topic title: <b>");
	add_text(new_post_page, 
						topic->get_topic());
	add_text(new_post_page, (char *) 
						"</b><br>"
					"Name: <input type=text name=\"name\"><br>"
					"Message: "
						"<br>"
							"<textarea name=\"text\" cols=80 rows=20>"
							"</textarea>"
						"<br>"
					"<button name=\"submit\" value=\"submit\" type=\"submit\">"
						"Add reply"
					"</button>"
				"</form>"
			"</td></tr></table>"
		"</center>");
	
	// The end of the board:
	add_text(new_post_page, board_end);
	
	return new_post_page;
}

// Returns an HTML page containing a thank you message and a clickable link to
// the main page.
data_buffer	get_thank_you_page(void)
{
	data_buffer	thank_you_page;
	
	thank_you_page.data = NULL;
	thank_you_page.len = 0;
	
	// The haggle logo and start of the board:
	add_text(thank_you_page, board_start);
	
	add_text(thank_you_page, (char *) 
		"<center>"
			"Thank you for your posting. "
			"<a href=\"/\">Click here</a> to return to the board."
		"</center>");
	
	// The end of the board:
	add_text(thank_you_page, board_end);
	
	return thank_you_page;
}

// Returns an HTML page containing a list of all the posts in a specific topic
data_buffer	get_topic_page(bbs_topic *topic)
{
	data_buffer					topic_page;
	std::list<bbs_posting *>	posts;
	char						str[32];
	
	topic_page.data = NULL;
	topic_page.len = 0;
	
	// The haggle logo and start of the board:
	add_text(topic_page, board_start);
	
	// Add "Add reply" button/text
	add_text(topic_page, (char *) 
		"<a href=\"/new_post_");
	sprintf(str, "%ld", topic->get_id());
	add_text(topic_page, 
				str); 

	// Start table:
	add_text(topic_page, (char *) 
			"\">"
			"Add reply"
		"</a>"
		"<center>"
			"<table width=100% border=1>");
	
	// Add table headings:
	add_text(topic_page, (char *) 
				"<tr>"
					"<th>"
						"Author:"
					"</th>"
					"<th width=80%>"
						"Message:"
					"</th>"
				"</tr>");
	
	// Add table content:
	posts = topic->get_posts();
	for(std::list<bbs_posting *>::iterator it = posts.begin();
		it != posts.end();
		it++)
	{
		add_text(topic_page, (char *) 
				"<tr>"
					"<td valign=top>"
						"<center>"
							"<b>");
		add_text(topic_page, 
						(*it)->get_author());
		add_text(topic_page,(char *) 
							"</b><br>"
							"<br>"
							"<i>");
		add_text(topic_page, 
						(*it)->get_date_of_post());
		add_text(topic_page,(char *) 
							"</i>"
						"</center>"
					"</td>"
					"<td valign=top>");
		add_text(topic_page, 
						(*it)->get_text());
		add_text(topic_page, (char *) 
					"</td>"
				"</tr>");
	}
	// Add table ending:
	add_text(topic_page, (char *) "</table></center>");
	
	// The end of the board:
	add_text(topic_page, board_end);
	
	return topic_page;
}

// Returns an HTML page containing a list of all the topics on the haggle board
data_buffer	get_board(void)
{
	data_buffer	board_page;
	
	board_page.data = NULL;
	board_page.len = 0;
	
	// The haggle logo and start of the board:
	add_text(board_page, board_start);
	
	add_text(board_page, (char *) 
		// Add "create new topic" button/text:
		"<a href=\"/new_topic\">Create new topic</a>"
		// Add table start:
		"<center>"
			"<table width=100% border=1>");
	
	// Add table headings:
	add_text(board_page, (char *) 
				"<tr>"
					"<th width=50%>Topic:</th>"
					"<th>Replies:</th>"
					"<th>Author</th>"
					"<th>Last post:</th>"
				"</tr>");
	
	// Add the actual board:
	for(std::list<bbs_topic *>::iterator it = topics.begin();
		it != topics.end();
		it++)
	{
		char	str[32];
		
		add_text(board_page, (char *) 
				"<tr>"
					"<td>"
						"<a href=\"/topic_");
		sprintf(str,		"%ld", (*it)->get_id());
		add_text(board_page, str);
		add_text(board_page, (char *) 
							"\">");
		add_text(board_page, 
							(*it)->get_topic());
		add_text(board_page, (char *) 
						"</a>"
					"</td>"
					"<td align=center>");
		sprintf(str, (char *) 
						"%ld",
				(long) (*it)->get_posts().size()-1);
		add_text(board_page, str);
		
		add_text(board_page, (char *) 
					"</td>"
					"<td align=center>");
		add_text(board_page, 
						(*it)->get_posts().front()->get_author());
		add_text(board_page, (char *) 
					"</td>"
					"<td align=center>");
		add_text(board_page, 
						(*it)->get_posts().back()->get_date_of_post());
		
		add_text(board_page, (char *) 
					"</td>"
				"</tr>");
	}
	
	// Add table end:
	add_text(board_page, (char *) 
			"</table>"
		"</center>");
	
	// The end of the board:
	add_text(board_page, board_end);
	
	return board_page;
}

// GET HTTP command processing:
void httpd_process_GET(char *resource_name)
{
	if(strcmp(resource_name, (char *) "/") == 0)
	{
		// Present the main board page:
		data_buffer index_html;
		
		index_html = get_board();
		
		http_reply_data(
			index_html.data, 
			index_html.len, 
			(char *) "text/html; charset=UTF-8");
		free(index_html.data);
	}else if(strncmp(resource_name, (char *) "/topic_", 7) == 0)
	{
		bbs_topic	*topic;
		
		// Present the given topic, if it exists:
		topic = get_topic_by_id(atoi(&(resource_name[7])));
		if(topic != NULL)
		{
			data_buffer topic_html;
			
			topic_html = get_topic_page(topic);
			
			http_reply_data(
				topic_html.data, 
				topic_html.len, 
				(char *) "text/html; charset=UTF-8");
			free(topic_html.data);
		}else{
			http_reply_err(404);
		}
	}else if(strncmp(resource_name, (char *) "/new_topic", 10) == 0)
	{
		data_buffer new_topic_html;
		
		// Present a "create new topic" form
		new_topic_html = get_new_topic_page();
		
		http_reply_data(
			new_topic_html.data, 
			new_topic_html.len, 
			(char *) "text/html; charset=UTF-8");
		free(new_topic_html.data);
	}else if(strncmp(resource_name, (char *) "/new_post_", 10) == 0)
	{
		bbs_topic	*topic;
		
		// Present the given topic, if it exists:
		topic = get_topic_by_id(atoi(&(resource_name[10])));
		if(topic != NULL)
		{
			data_buffer new_post_html;
			
			// Present an "Add reply" form
			new_post_html = get_new_post_page(topic);
			
			http_reply_data(
				new_post_html.data, 
				new_post_html.len, 
				(char *) "text/html; charset=UTF-8");
			free(new_post_html.data);
		}else{
			http_reply_err(404);
		}
	}else if(strcmp(
				&(resource_name[strlen(resource_name)-4]), 
				(char *) ".png") == 0 ||
			 strcmp(
			 	&(resource_name[strlen(resource_name)-4]), 
			 	(char *) ".PNG") == 0)
	{
		data_buffer	buf;
		
		if(resource_name[0] == '/')
			buf = load_file(&(resource_name[1]));
		else
			buf = load_file(resource_name);
		if(buf.len == 0)
			http_reply_err(404);
		else{
			http_reply_data(
				buf.data, 
				buf.len, 
				(char *) "image/png");
			free(buf.data);
		}
	}else
		http_reply_err(404);
}

// POST HTTP command processing:
void httpd_process_POST(char *resource_name, char *post_data, long data_len)
{
	if(strcmp(resource_name, (char *) "/new_topic") == 0)
	{
		data_buffer	name, topic, text, page;
		
		page.data = NULL;
		
		name = 
			httpd_post_data_get_data_for_input(
				(char *) "name", 
				post_data, 
				data_len);
		if(name.data == NULL)
		{
			http_reply_err(400);
			goto new_topic_done;
		}
		topic = 
			httpd_post_data_get_data_for_input(
				(char *) "topic", 
				post_data, 
				data_len);
		if(topic.data == NULL)
		{
			http_reply_err(400);
			goto new_topic_done;
		}
		text = 
			httpd_post_data_get_data_for_input(
				(char *) "text", 
				post_data, 
				data_len);
		if(text.data == NULL)
		{
			http_reply_err(400);
			goto new_topic_done;
		}
		
		add_post(
			topic.data, 
			text.data, 
			name.data);
		
		page = get_thank_you_page();
		http_reply_data(
			page.data, 
			page.len, 
			(char *) "text/html; charset=UTF-8");
		
new_topic_done:
		if(page.data != NULL)
			free(page.data);
	}else if(strncmp(resource_name, (char *) "/add_reply_", 11) == 0)
	{
		data_buffer	name, text, page;
		bbs_topic	*topic;
		
		page.data = NULL;
		
		topic = get_topic_by_id(atoi(&(resource_name[11])));
		if(topic == NULL)
		{
			http_reply_err(404);
			goto new_post_done;
		}
		
		name = 
			httpd_post_data_get_data_for_input(
				(char *) "name", 
				post_data, 
				data_len);
		if(name.data == NULL)
		{
			http_reply_err(400);
			goto new_post_done;
		}
		text = 
			httpd_post_data_get_data_for_input(
				(char *) "text", 
				post_data, 
				data_len);
		if(text.data == NULL)
		{
			http_reply_err(400);
			goto new_post_done;
		}
		
		add_post(
			topic->get_topic(), 
			text.data, 
			name.data);
		
		page = get_thank_you_page();
		http_reply_data(
			page.data, 
			page.len, 
			(char *) "text/html; charset=UTF-8");
new_post_done:
		if(page.data != NULL)
			free(page.data);
	}else{
		printf((char *) "POST for: \"%s\"\n", resource_name);
		http_reply_err(404);
	}
}

// Signal handler: shuts down the program nicely upon Ctrl-C, rather than
// letting the system crash with open network ports, etc.
static void signal_handler(int signal)
{
	switch(signal)
	{
#if defined(OS_UNIX)
		case SIGKILL:
		case SIGHUP:
#endif
		case SIGINT: // Not supported by OS_WINDOWS?
		case SIGTERM:
			httpd_shutdown();
		break;
		
		default:
		break;
	}
}

int main(void)
{
	// This is to catch Ctrl-C:
#if defined(OS_UNIX)
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
#endif
#if defined(OS_WINDOWS)
	signal(SIGTERM, &signal_handler);
	signal(SIGINT, &signal_handler);
#elif defined(OS_UNIX)
	sigact.sa_handler = &signal_handler;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
#endif
	
	// Find Haggle:
	if(haggle_handle_get("Haggle BBS", &haggle_) != HAGGLE_NO_ERROR)
		goto fail_haggle;
	
	// Start the HTTP library:
	if(!httpd_startup(8082))
		goto fail_startup;
	
	// FIXME: Tell haggle we want to know about haggle shutting down
	// Tell haggle we want to know about new data objects:
	haggle_ipc_register_event_interest(
		haggle_, 
		LIBHAGGLE_EVENT_NEW_DATAOBJECT, 
		onDataObject);
	
	// Start the haggle event loop:
	haggle_event_loop_run_async(haggle_);
	
	// Add the BBS attributes to our interests:
	haggle_ipc_add_application_interest(
		haggle_, 
		haggle_bbs_attribute_name, 
		haggle_bbs_attribute_post_value);
	
	// Start accepting connections:
	httpd_accept_connections();
	
	// I suppose this should be done:
	haggle_event_loop_stop(haggle_);
	
	// Tell haggle we don't want to know anything else:
	haggle_handle_free(haggle_);
	
	return 0;
	if(0)
fail_startup:
		printf((char *) "Unable to start HTTPD library\n");
	haggle_handle_free(haggle_);
	if(0)
fail_haggle:
		printf((char *) "Unable to get haggle handle\n");
	return 1;
}

/*
	This function inserts space for "how_many" chars "where" chars into the
	string "txt", which has "txt_len" characters already.
*/
bool insert_chars(char **txt, long txt_len, long where, long how_many)
{
	long	j;
	char	*tmp;
	
	tmp = (char *) realloc(*txt, txt_len + 3);
	if(tmp == NULL)
		return false;
	
	*txt = tmp;
	
	for(j = txt_len; j > where + how_many; j--)
		tmp[j] = tmp[j-how_many];
	
	return true;
}

/*
	This function removes all '<', '>', '"', and '&' characters from the given 
	text, and replaces them (when all goes well) with their HTML versions: 
	"&lt;", "&gt;", "&quot;" and "&amp;". If all does not go well, they are 
	replaced with "[", "]", "#" and "#" respectively. 
	
	If fix_newlines is true, this function also attempts to change all "\n", 
	"\r" and "\r\n" into HTML "<br>" tags.
	
	This is used to keep posters from posting malicious HTML code in a post and 
	launch any kind of attack against the other Haggle BBS users. Also to
	make sure the text doesn't cause any problems.
*/
void board_text_fix(char **txt, long *txt_len, bool fix_newlines)
{
	long	i;
	
	for(i = 0; i < *txt_len; i++)
	{
		if((*txt)[i] == '<')
		{
			if(insert_chars(txt, *txt_len, i, 3))
			{
				*txt_len += 3;
				(*txt)[i] = '&';i++;
				(*txt)[i] = 'l';i++;
				(*txt)[i] = 't';i++;
				(*txt)[i] = ';';
			}else{
				(*txt)[i] = '[';
			}
		}
		if((*txt)[i] == '>')
		{
			if(insert_chars(txt, *txt_len, i, 3))
			{
				*txt_len += 3;
				(*txt)[i] = '&';i++;
				(*txt)[i] = 'g';i++;
				(*txt)[i] = 't';i++;
				(*txt)[i] = ';';
			}else{
				(*txt)[i] = ']';
			}
		}
		if((*txt)[i] == '"')
		{
			if(insert_chars(txt, *txt_len, i, 5))
			{
				*txt_len += 5;
				(*txt)[i] = '&';i++;
				(*txt)[i] = 'q';i++;
				(*txt)[i] = 'u';i++;
				(*txt)[i] = 'o';i++;
				(*txt)[i] = 't';i++;
				(*txt)[i] = ';';
			}else{
				(*txt)[i] = '#';
			}
		}
		if((*txt)[i] == '&')
		{
			if(insert_chars(txt, *txt_len, i, 4))
			{
				*txt_len += 4;
				(*txt)[i] = '&';i++;
				(*txt)[i] = 'a';i++;
				(*txt)[i] = 'm';i++;
				(*txt)[i] = 'p';i++;
				(*txt)[i] = ';';
			}else{
				(*txt)[i] = '#';
			}
		}
		if(fix_newlines)
		{
			if((*txt)[i] == '\n')
			{
				if(insert_chars(txt, *txt_len, i, 3))
				{
					*txt_len += 3;
					(*txt)[i] = '<';i++;
					(*txt)[i] = 'b';i++;
					(*txt)[i] = 'r';i++;
					(*txt)[i] = '>';
				}
			}
			if((*txt)[i] == '\r')
			{
				if(i + 1 < *txt_len)
				{
					if((*txt)[i+1] == '\n')
					{
						if(insert_chars(txt, *txt_len, i, 2))
						{
							*txt_len += 2;
							(*txt)[i] = '<';i++;
							(*txt)[i] = 'b';i++;
							(*txt)[i] = 'r';i++;
							(*txt)[i] = '>';
						}
					}else{
						if(insert_chars(txt, *txt_len, i, 3))
						{
							*txt_len += 3;
							(*txt)[i] = '<';i++;
							(*txt)[i] = 'b';i++;
							(*txt)[i] = 'r';i++;
							(*txt)[i] = '>';
						}
					}
				}else{
					if(insert_chars(txt, *txt_len, i, 3))
					{
						*txt_len += 3;
						(*txt)[i] = '<';i++;
						(*txt)[i] = 'b';i++;
						(*txt)[i] = 'r';i++;
						(*txt)[i] = '>';
					}
				}
			}
		}
	}
}
