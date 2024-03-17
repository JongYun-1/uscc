//
//  ParseStmt.cpp
//  uscc
//
//  Implements all of the recursive descent parsing
//  functions for statement grammar rules.
//
//---------------------------------------------------------
//  Copyright (c) 2014, Sanjay Madhav
//  All rights reserved.
//
//  This file is distributed under the BSD license.
//  See LICENSE.TXT for details.
//---------------------------------------------------------

#include "Parse.h"
#include "Symbols.h"

using namespace uscc::parse;
using namespace uscc::scan;

using std::shared_ptr;
using std::make_shared;

shared_ptr<ASTDecl> Parser::parseDecl()
{
	shared_ptr<ASTDecl> retVal;
	// A decl MUST start with int or char
	if (peekIsOneOf({Token::Key_int, Token::Key_char}))
	{
		Type declType = Type::Void;
		if (peekToken() == Token::Key_int)
		{
			declType = Type::Int;
		}
		else
		{
			declType = Type::Char;
		}
		
		consumeToken();
		
		// Set this to @@variable for now. We'll later change it
		// assuming we parse the identifier properly
		Identifier* ident = mSymbols.getIdentifier("@@variable");
		
		// Now we MUST get an identifier so go into a try
		try
		{
			if (peekToken() != Token::Identifier)
			{
				throw ParseExceptMsg("Type must be followed by identifier");
			}
			
			auto token = getTokenTxt();
			if (mSymbols.isDeclaredInScope(token))
				reportSemantError(std::string("Invalid redeclaration of identifier '") + std::string(token) + "'");

			ident = mSymbols.createIdentifier(token); // if redeclaration, nothing will happen
			
			consumeToken();
			
			// Is this an array declaration?
			if (peekAndConsume(Token::LBracket))
			{
				shared_ptr<ASTConstantExpr> constExpr;
				if (declType == Type::Int)
				{
					declType = Type::IntArray;
					
					// int arrays must have a constant size defined,
					// because USC doesn't support initializer lists
					constExpr = parseConstantFactor();
					if (!constExpr)
					{
						reportSemantError("Int arrays must have a defined constant size");
					}
					
					if (constExpr)
					{
						int count = constExpr->getValue();
						if (count <= 0 || count > 65536)
						{
							reportSemantError("Arrays must have a min of 1 and a max of 65536 elements");
						}
						ident->setArrayCount(count);
					}
					else
					{
						ident->setArrayCount(0);
					}
				}
				else
				{
					declType = Type::CharArray;
					
					// For character, we support both constant size or
					// implict size if it's assigned to a constant string
					constExpr = parseConstantFactor();
					if (constExpr)
					{
						int count = constExpr->getValue();
						if (count <= 0 || count > 65536)
						{
							reportSemantError("Arrays must have a min of 1 and a max of 65536 elements");
						}
						ident->setArrayCount(count);
					}
					else
					{
						// We'll determine this later in the parse
						ident->setArrayCount(0);
					}
				}
				
				matchToken(Token::RBracket);
			}
			
			ident->setType(declType);
			
			shared_ptr<ASTExpr> assignExpr;
			
			// Optionally, this decl may have an assignment
			// PA2 mine
			int col = mColNumber;
				
			if (peekAndConsume(Token::Assign))
			{
				// We don't allow assignment for int arrays
				if (declType == Type::IntArray)
				{
					reportSemantError("USC does not allow assignment of int array declarations");
				}
				
				assignExpr = parseExpr();
				if (!assignExpr)
				{
					throw ParseExceptMsg("Invalid expression after = in declaration");
				}
				
				// PA2: Type checks
				if (ident->getType() == Type::Char)
				{
					if (assignExpr->getType() == Type::Int)
						assignExpr = intToChar(assignExpr);
				}
				// after conversion if still not match
				if (ident->getType() != assignExpr->getType())
				{
					std::string err("Cannot assign an expression of type ");
					err += getTypeText(assignExpr->getType());
					err += " to ";
					err += getTypeText(ident->getType());
					reportSemantError(err, col);
				}

				// If this is a character array, we need to do extra checks
				if (ident->getType() == Type::CharArray)
				{
					ASTStringExpr* strExpr = dynamic_cast<ASTStringExpr*>(assignExpr.get());
					if (strExpr != nullptr)
					{
						// If we have a declared size, we need to make sure
						// there's enough room to fit the requested string.
						// Otherwise, we need to set our size
						if (ident->getArrayCount() == 0)
						{
							ident->setArrayCount(strExpr->getLength() + 1);
						}
						else if (ident->getArrayCount() < (strExpr->getLength() + 1))
						{
							reportSemantError("Declared array cannot fit string");
						}
					}
				}
			}
			else if (ident->getType() == Type::CharArray && ident->getArrayCount() == 0)
			{
				reportSemantError("char array must have declared size if there's no assignment");
			}
			
			matchToken(Token::SemiColon);
			
			retVal = make_shared<ASTDecl>(*ident, assignExpr);
		}
		catch (ParseExcept& e)
		{
			reportError(e);
			
			// Skip all the tokens until the next semi-colon
			consumeUntil(Token::SemiColon);
			
			if (peekToken() == Token::EndOfFile)
			{
				throw EOFExcept();
			}
			
			// Grab this semi-colon, also
			consumeToken();
			
			// Put in a decl here with the bogus identifier
			// "@@error". This is so the parse will continue to the
			// next decl, if there is one.
			retVal = make_shared<ASTDecl>(*(ident));
		}
	}
	
	return retVal;
}

shared_ptr<ASTStmt> Parser::parseStmt()
{
	shared_ptr<ASTStmt> retVal;
	try
	{
		// NOTE: AssignStmt HAS to go before ExprStmt!!
		// Read comments in AssignStmt for why.
		if ((retVal = parseCompoundStmt()))
			;
		else if ((retVal = parseAssignStmt()))
			;
		else if ((retVal = parseReturnStmt()))
			;
		else if ((retVal = parseWhileStmt()))
			;
		else if ((retVal = parseExprStmt()))
			;
		else if ((retVal = parseNullStmt()))
			;
		else if ((retVal = parseIfStmt()))
			;
		// PA1: Add additional cases
		
		else if (peekIsOneOf({Token::Key_int, Token::Key_char}))
		{
			throw ParseExceptMsg("Declarations are only allowed at the beginning of a scope block");
		}
	}
	catch (ParseExcept& e)
	{
		reportError(e);
		
		// Skip all the tokens until the next semi-colon
		consumeUntil(Token::SemiColon);
		
		if (peekToken() == Token::EndOfFile)
		{
			throw EOFExcept();
		}
		
		// Grab this semi-colon, also
		consumeToken();
		
		// Put in a null statement here
		// so we can try to continue.
		retVal = make_shared<ASTNullStmt>();
	}
	
	return retVal;
}

// If the compound statement is a function body, then the symbol table scope
// change will happen at a higher level, so it shouldn't happen in
// parseCompoundStmt.
shared_ptr<ASTCompoundStmt> Parser::parseCompoundStmt(bool isFuncBody)
{
	shared_ptr<ASTCompoundStmt> retVal;
	
	// PA1: Implement
	if (peekAndConsume(Token::LBrace))
	{
		// PA2 mine
		if (!isFuncBody)
			mSymbols.enterScope();

		retVal = make_shared<ASTCompoundStmt>();
		shared_ptr<ASTDecl> decl;
		decl = parseDecl();
		while (decl != nullptr)
		{
			retVal->addDecl(decl);
			decl = parseDecl();
		}

		shared_ptr<ASTStmt> stmt, lastStmt; // preserve the last statment for check
		stmt = parseStmt();
		while (stmt != nullptr)
		{
			retVal->addStmt(stmt);
			lastStmt = stmt;
			stmt = parseStmt();
		}

		// PA2 mine
		if (!std::dynamic_pointer_cast<ASTReturnStmt>(lastStmt) && isFuncBody)
		{
			if (mCurrReturnType == Type::Void)
				retVal->addStmt(make_shared<ASTReturnStmt>(nullptr));
			else
				reportSemantError("USC requires non-void functions to end with a return");
		}

		matchToken(Token::RBrace);

		// PA2 mine
		if (!isFuncBody)
			mSymbols.exitScope();
	}
	
	return retVal;
}

shared_ptr<ASTStmt> Parser::parseAssignStmt()
{
	shared_ptr<ASTStmt> retVal;
	shared_ptr<ASTArraySub> arraySub;
	
	if (peekToken() == Token::Identifier)
	{
		Identifier* ident = getVariable(getTokenTxt());
		
		consumeToken();
		
		// Now let's see if this is an array subscript
		if (peekAndConsume(Token::LBracket))
		{
			try
			{
				shared_ptr<ASTExpr> expr = parseExpr();
				if (!expr)
				{
					throw ParseExceptMsg("Valid expression required inside [ ].");
				}
				
				arraySub = make_shared<ASTArraySub>(*ident, expr);
			}
			catch (ParseExcept& e)
			{
				// If this expr is bad, consume until RBracket
				reportError(e);
				consumeUntil(Token::RBracket);
				if (peekToken() == Token::EndOfFile)
				{
					throw EOFExcept();
				}
			}
			
			matchToken(Token::RBracket);
		}
		
		// Just because we got an identifier DOES NOT necessarily mean
		// this is an assign statement.
		// This is because there is a common left prefix between
		// AssignStmt and an ExprStmt with statements like:
		// id ;
		// id [ Expr ] ;
		// id ( FuncCallArgs ) ;
		
		// So... We see if the next token is a =. If it is, then this is
		// an AssignStmt. Otherwise, we set the "unused" variables
		// so parseFactor will later find it and be able to match
		int col = mColNumber;
		if (peekAndConsume(Token::Assign))
		{
			shared_ptr<ASTExpr> expr = parseExpr();
			
			if (!expr)
			{
				throw ParseExceptMsg("= must be followed by an expression");
			}
			
			// If we matched an array, we want to make an array assign stmt
			if (arraySub)
			{
				// Make sure the type of this expression matches the declared type
				Type subType;
				if (arraySub->getType() == Type::IntArray)
				{
					subType = Type::Int;
				}
				else
				{
					subType = Type::Char;
				}
				if (mCheckSemant && subType != expr->getType())
				{
					// We can do a conversion if it's from int to char
					if (subType == Type::Char &&
						expr->getType() == Type::Int)
					{
						expr = intToChar(expr);
					}
					else
					{
						std::string err("Cannot assign an expression of type ");
						err += getTypeText(expr->getType());
						err += " to ";
						err += getTypeText(subType);
						reportSemantError(err, col);
					}
				}
				retVal = make_shared<ASTAssignArrayStmt>(arraySub, expr);
			}
			else
			{
				// PA2: Check for semantic errors
				if (ident->getType() == Type::Char)
				{
					if (expr->getType() == Type::Int)
						expr = intToChar(expr);
				}

				// after conversion if still not match
				if (ident->getType() != expr->getType())
				{
					std::string err("Cannot assign an expression of type ");
					err += getTypeText(expr->getType());
					err += " to ";
					err += getTypeText(ident->getType());
					reportSemantError(err, col);
				}

				if (ident->getType() == Type::IntArray || ident->getType() == Type::CharArray)
					reportSemantError("Reassignment of arrays is not allowed", col);

				retVal = make_shared<ASTAssignStmt>(*ident, expr);
			}
			
			matchToken(Token::SemiColon);
		}
		else
		{
			// We either have an unused array, or an unused ident
			if (arraySub)
			{
				mUnusedArray = arraySub;
			}
			else
			{
				mUnusedIdent = ident;
			}
		}
	}
	
	return retVal;
}

shared_ptr<ASTIfStmt> Parser::parseIfStmt()
{
	shared_ptr<ASTIfStmt> retVal;
	
	// PA1: Implement
	if (peekAndConsume(Token::Key_if))
	{
		matchToken(Token::LParen);
		auto expr = parseExpr();
		if (!expr)
			throw ParseExceptMsg("Invalid condition for if statement");
		matchToken(Token::RParen);

		auto stmt = parseStmt();
		shared_ptr<ASTStmt> elseStmt;
		if (peekAndConsume(Token::Key_else))
			elseStmt = parseStmt();
		retVal = make_shared<ASTIfStmt>(expr, stmt, elseStmt);
	}
	
	return retVal;
}

shared_ptr<ASTWhileStmt> Parser::parseWhileStmt()
{
	shared_ptr<ASTWhileStmt> retVal;
	
	// PA1: Implement
	if (peekAndConsume(Token::Key_while))
	{
		shared_ptr<ASTExpr> expr;
		shared_ptr<ASTStmt> stmt;
		matchToken(Token::LParen);
		expr = parseExpr();
		if (!expr)
			throw ParseExceptMsg("Invalid condition for while statement");
		matchToken(Token::RParen);

		stmt = parseStmt();
		retVal = make_shared<ASTWhileStmt>(expr, stmt);
	}
	
	return retVal;
}

shared_ptr<ASTReturnStmt> Parser::parseReturnStmt()
{
	shared_ptr<ASTReturnStmt> retVal;
	
	// PA1: Implement
	if (peekAndConsume(Token::Key_return))
	{
		if (peekIsOneOf({Token::SemiColon}))
		{
			retVal = make_shared<ASTReturnStmt>(nullptr);

			// PA2 mine
			if (mCurrReturnType != Type::Void)
				reportSemantError("Invalid empty return in non-void function");
			consumeToken();
		}
		else
		{
			// PA2 mine
			int col = mColNumber;
			auto expr = parseExpr();
			
			// PA2 mine
			if (mCurrReturnType == Type::Char)
			{
				if (expr->getType() == Type::Int)
					expr = intToChar(expr);
			}

			// after conversion if still not match
			if (mCurrReturnType != expr->getType())
			{
				std::string err("Expected type ");
				err += getTypeText(mCurrReturnType);
				err += " in return statement";
				reportSemantError(err, col);
			}

			retVal = make_shared<ASTReturnStmt>(expr);
			matchToken(Token::SemiColon);
		}
	}
	
	return retVal;
}

shared_ptr<ASTExprStmt> Parser::parseExprStmt()
{
	shared_ptr<ASTExprStmt> retVal;
	
	// PA1: Implement
	auto e = parseExpr();
	if (e)
	{
		retVal = make_shared<ASTExprStmt>(e);
		matchToken(Token::SemiColon);
	}
	
	return retVal;
}

shared_ptr<ASTNullStmt> Parser::parseNullStmt()
{
	shared_ptr<ASTNullStmt> retVal;
	
	// PA1: Implement
	if (peekAndConsume(Token::SemiColon))
		retVal = make_shared<ASTNullStmt>();
	
	return retVal;
}
