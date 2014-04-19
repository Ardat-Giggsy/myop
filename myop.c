/*
 * elevator myop
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>

struct myop_data {
	struct list_head queue;
};

static void myop_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int myop_dispatch(struct request_queue *q, int force)
{
	struct myop_data *nd = q->elevator->elevator_data;

	if (!list_empty(&nd->queue)) {
		struct request *rq;
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void myop_add_request(struct request_queue *q, struct request *rq)
{
	struct myop_data *nd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &nd->queue);
}

static int myop_queue_empty(struct request_queue *q)
{
	struct myop_data *nd = q->elevator->elevator_data;

	return list_empty(&nd->queue);
}

static struct request *
myop_former_request(struct request_queue *q, struct request *rq)
{
	struct myop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
myop_latter_request(struct request_queue *q, struct request *rq)
{
	struct myop_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *myop_init_queue(struct request_queue *q)
{
	struct myop_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	return nd;
}

static void myop_exit_queue(struct elevator_queue *e)
{
	struct myop_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_myop = {
	.ops = {
		.elevator_merge_req_fn		= myop_merged_requests,
		.elevator_dispatch_fn		= myop_dispatch,
		.elevator_add_req_fn		= myop_add_request,
		.elevator_queue_empty_fn	= myop_queue_empty,
		.elevator_former_req_fn		= myop_former_request,
		.elevator_latter_req_fn		= myop_latter_request,
		.elevator_init_fn		= myop_init_queue,
		.elevator_exit_fn		= myop_exit_queue,
	},
	.elevator_name = "myop",
	.elevator_owner = THIS_MODULE,
};

static int __init myop_init(void)
{
	elv_register(&elevator_myop);

	return 0;
}

static void __exit myop_exit(void)
{
	elv_unregister(&elevator_myop);
}

module_init(myop_init);
module_exit(myop_exit);


MODULE_AUTHOR("Rui Liu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("my I/O scheduler to do something different from what's already in Linux");
