struct GlobalVars
{

};

__section(".text") int test_function(__reg("a6") struct GlobalVars* globals)
{
	return 0x1337;
}
